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

# get kerberos ticket 
kinit --renewable -k -t "${KEYTAB_FILE}" "${KERBEROS_PRINCIPAL}"

# copy files to the /storage
if ! ssh "${STORAGE_USER}@${STORAGE_SERVER}" mkdir -pm 700 "saxsfit/requests/${request_id}/pbs" &> /dev/null; then
	exit_error "${RETURN_SERVER_ERROR}" "Cannot create request directory on the storage"
fi
 
if ! scp -qr "${request_dir}/config.request" "${request_dir}/results" "${request_dir}/workdir" "${STORAGE_USER}@${STORAGE_SERVER}:${STORAGE_DIR}/${request_id}"; then
	exit_error "${RETURN_SERVER_ERROR}" "Cannot copy files to the storage"
fi

exit "${RETURN_OK}"
