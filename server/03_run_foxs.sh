#!/bin/bash
set -u

# XXX: this phase is not required anymore

exit "${RETURN_OK}"

source common
source config.server

if [ $# -ne 1 ]; then
	echo "Missing request id argument"
	exit "${RETURN_SERVER_ERROR}"
fi

request_id=$1
request_dir="${REQUESTS_DIR}/${request_id}" 

# renew kerberos ticket
kinit -R "${KERBEROS_PRINCIPAL}"

# submit jobs to the PBS
shopt -s nullglob
for file in $(find "${request_dir}/workdir/structures/" -name '*.pdb'); do
	structure_file="$(basename "${file}")"
	structure_base="$(basename "${file}" .pdb)"
	qsub_options=(${FOXS_QSUB_ARGS} -N "saxsfit[${request_id}:foxs_job]:${structure_base}" -v "request_id=${request_id},structure_file=${structure_file},storage_dir=${STORAGE_FULL_DIR}" -w "${STORAGE_FULL_DIR}/requests/${request_id}/pbs" "foxs_job.sh")

	job_id="$(qsub "${qsub_options[@]}")"
	[ $? -eq 0 ] || exit_error "${RETURN_SERVER_ERROR}" "Cannot submit job to PBS: qsub" "${qsub_options[@]}"
	
	log_info "Submitted to PBS: saxsfit[${request_id}:foxs_job]:${structure_base} (${job_id})"
	echo "${job_id}" >> "${request_dir}/foxs_jobs_running"
done
shopt -u nullglob

# check for jobs' end
while true; do
	all_done="true"
	for job_id in $(< "${request_dir}/foxs_jobs_running"); do
		job_state="$(qstat -f "${job_id}" | grep "job_state" | sed 's/.*= //')"
		if [ "${job_state}" != "C" ]; then
			all_done="false"
			break
		else
			job_exit_code="$(qstat -f "${job_id}" | grep "exit_status" | sed 's/.*= //')"
			[ "${job_exit_code}" -eq 0 ] || exit_error "${RETURN_SERVER_ERROR}" "Job execution was unsuccessful: ${job_id}"

			log_info "PBS job ended successfully (${job_id})"

			# job completed successfully, remove job_id from the list so that it won't be checked again
			sed -i "/^${job_id}$/d" "${request_dir}/foxs_jobs_running"
		fi
	done
	
	if [ "${all_done}" = "true" ]; then
		break
	else
		sleep "${FOXS_JOBS_CHECK_INTERVAL}"
	fi
done

exit "${RETURN_OK}"
