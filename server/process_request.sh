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
echo "checked" > "${request_dir}/state"

# 1. preprocess input data
./01_preprocess.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"preprocessed\""
echo "preprocessed" > "${request_dir}/state"

# 2. prepare storage
./02_prepare_storage.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit ${retval}

log_info "Request changed state to \"storage_ready\""
echo "storage_ready" > "${request_dir}/state"

# 3. compute SAXS curves
./03_run_foxs.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"foxs_completed\""
echo "foxs_completed" > "${request_dir}/state"

# 4. run optimization
./04_run_optim.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"optim_completed\""
echo "optim_completed" > "${request_dir}/state"

# 5. clean things up
./05_clean.sh "${request_id}"
retval=$?
[ "${retval}" -eq 0 ] || exit "${retval}"

log_info "Request changed state to \"done\""
echo "done" > "${request_dir}/state"

exit "${RETURN_OK}"
