#!/bin/bash
set -u

source common
source config.server

while true; do
	for dir in $(find "${REQUESTS_DIR}" -mindepth 1 -maxdepth 1 -type d); do
		if [ "$(cat "${dir}/state" 2>/dev/null)" == "uploaded" ]; then
			echo "started" > "${dir}/state"
			request_id="$(basename "${dir}")"
			log_info "Request processing started"
			./process_request.sh "${request_id}" &
		fi
	done
	sleep "${CHECK_FOR_REQUESTS_INTERVAL}"
done
