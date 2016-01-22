#!/bin/bash
set -u
set -x 

module add openmpi-1.8.2-intel

source "${storage_dir}/config.server"
source "${storage_dir}/requests/${request_id}/config.request"

request_dir="${storage_dir}/requests/${request_id}"
request_workdir="${storage_dir}/requests/${request_id}/workdir"

# copy application
cp "${storage_dir}/ensamble-fit" "${SCRATCHDIR}" || exit 1
cp "${storage_dir}/foxs" "${SCRATCHDIR}" || exit 1

# copy required data
cp ${request_workdir}/structures/*.pdb "${SCRATCHDIR}" || exit 1
cp "${request_workdir}/saxs.dat" "${SCRATCHDIR}" || exit 1

cd "${SCRATCHDIR}" || exit 1

# perform optimization

structures_count="$(find . -name '*.pdb' | wc -l)"

PATH=.:$PATH
mpirun ./ensamble-fit -n "${structures_count}" -m saxs.dat -s "${OPTIM_STEPS}" -y "${OPTIM_SYNC_STEPS}" -l "${OPTIM_PARAM_ALPHA}" -b "${OPTIM_PARAM_BETA}" -g "${OPTIM_PARAM_GAMMA}"  -L -p structure00 >> "${request_dir}/results"
retval="$?"

# clean things up
# rm -rf "${SCRATCHDIR}/"*

exit "${retval}"
