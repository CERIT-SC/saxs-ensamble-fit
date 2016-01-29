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

PATH=${SCRATCHDIR}:$PATH
TMPDIR=${SCRATCHDIR}
export TMPDIR
mpirun ./ensamble-fit -n "${structures_count}" -m saxs.dat -s "${OPTIM_STEPS}" -y "${OPTIM_SYNC_STEPS}" -l "${OPTIM_PARAM_ALPHA}" -b "${OPTIM_PARAM_BETA}" -g "${OPTIM_PARAM_GAMMA}"  -L -p structure00 >> "${request_dir}/results" &

pid=$!

while ps p $pid >/dev/null 2>/dev/null; do
	sleep $OPTIM_JOBS_CHECK_INTERVAL
	if test -f progress.dat; then
		cp progress.dat ${request_dir}
		set -- `grep c12: progress.dat`
		e=$2 w=$3

		mkdir ComputedCurves
		cd ComputedCurves
		ln ../saxs.dat .
		for i in `seq 1 $structures_count`; do
			n=`printf %02d $i`
			ln ../structure00$n.pdb .
			foxs -e $e -w $w structure00$n.pdb saxs.dat
			grep -v '^#' structure00${n}_saxs.dat | while read l; do
				set -- $l
				echo $1 $3 0.0 >>final_m${i}.pdb.dat 
			done 
		done
		cd ..
		tar cf ComputedCurves.tar ComputedCurves/final_m*.dat
		mv ComputedCurves.tar progress.dat ${request_dir}
		rm -rf ComputedCurves.tar ComputedCurves
	fi
done

wait $pid
retval="$?"

# clean things up
# rm -rf "${SCRATCHDIR}/"*

exit "${retval}"
