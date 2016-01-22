#!/bin/bash
set -u

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

qsub_options=(${OPTIM_QSUB_ARGS} -N "saxsfit[${request_id}:optim_job]" -v "request_id=${request_id},storage_dir=${STORAGE_FULL_DIR}" -w "${STORAGE_FULL_DIR}/requests/${request_id}/pbs" "optim_job.sh")

job_id="$(qsub "${qsub_options[@]}")"
[ $? -eq 0 ] || exit_error "${RETURN_SERVER_ERROR}" "Cannot submit job to PBS: qsub" "${qsub_options[@]}"

log_info "Submitted to PBS: saxsfit[${request_id}:optim_job] (${job_id})"

while true; do
	job_state="$(qstat -f "${job_id}" | grep "job_state" | sed 's/.*= //')"

	if ! ssh "${STORAGE_USER}@${STORAGE_SERVER}" "cat ${STORAGE_DIR}/${request_id}/results" | tail -1 > "${request_dir}/results"; then
		exit_error "${RETURN_SERVER_ERROR}" "Cannot obtain results from the storage"
	fi

	if [ "${job_state}" = "C" ]; then
		job_exit_code="$(qstat -f "${job_id}" | grep "exit_status" | sed 's/.*= //')"
		[ "${job_exit_code}" -eq 0 ] || exit_error "${RETURN_SERVER_ERROR}" "Job execution was unsuccessful: ${job_id}"
		
		log_info "PBS job ended successfully (${job_id})"
		break
	else
		sleep "${OPTIM_JOBS_CHECK_INTERVAL}"
	fi
done

exit "${RETURN_OK}"
