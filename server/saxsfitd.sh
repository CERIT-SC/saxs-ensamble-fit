#!/bin/bash
set -u

source common
source config.server

while true; do
	for dir in $(find "${REQUESTS_DIR}" -mindepth 2 -maxdepth 2 -type d); do
		if [ "$(grep ^status: "${dir}/result.dat" | sed 's/status://' 2>/dev/null)" == "accepted" ]; then
			update_results "${dir}/result.dat" status started
			set -- $(echo "${dir}" | tr / '\012' | tail -2)
			request_id="$1/$2"
			log_info "$request_id: Request processing started"
#			./process_request.sh "${request_id}" &
		fi
	done
	sleep "${CHECK_FOR_REQUESTS_INTERVAL}"
done
