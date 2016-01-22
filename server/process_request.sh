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

# 0. perform checks on input data
./00_check_data.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"checked\""
update_results "${request_dir}/result.dat" status "checked" 

# 1. preprocess input data
./01_preprocess.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"preprocessed\""
update_results "${request_dir}/result.dat" status "preprocessed" 

# 2. prepare storage
./02_prepare_storage.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit ${retval}

log_info "Request changed state to \"storage_ready\""
update_results "${request_dir}/result.dat" status "storage_ready" 

# 3. compute SAXS curves
./03_run_foxs.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"foxs_completed\""
update_results "${request_dir}/result.dat" status "foxs_completed" 

# 4. run optimization
./04_run_optim.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"optim_completed\""
update_results "${request_dir}/result.dat" status "optim_completed" 

# 5. clean things up
./05_clean.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"done\""
update_results "${request_dir}/result.dat" status "done" 

exit "${RETURN_OK}"
