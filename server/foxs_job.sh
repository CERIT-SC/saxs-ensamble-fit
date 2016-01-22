#!/bin/bash
set -u

feval()
{
	echo "$*" | bc -l
}

ftest()
{
        r="$(echo "$*" | bc -l)"
        test "${r}" = 1
}

module add openmpi

source "${storage_dir}/config.server"

request_workdir="${storage_dir}/requests/${request_id}/workdir"
# copy applications
cp "${storage_dir}/foxs" "${SCRATCHDIR}" || exit 1
cp "${storage_dir}/parse-map" "${SCRATCHDIR}" || exit 1

# copy required data
cp "${request_workdir}/structures/${structure_file}" "${SCRATCHDIR}" || exit 1
cp "${request_workdir}/saxs.dat" "${SCRATCHDIR}" || exit 1

cd "${SCRATCHDIR}"

# perform foxs calculation

structure_base="$(basename "${structure_file}" .pdb)"

# WORKAROUND: for q > 0.5 foxs requires -q parameter to be set
# max value in the experimental profile might not be included in the result -> use 1.5 factor
max_q="$(tail -1 saxs.dat | awk '{printf("%f\n", $1 * 1.5)}')"

e="${FOXS_E_MIN}"
while ftest "${e} <= ${FOXS_E_MAX}"; do
        w="${FOXS_W_MIN}"
        while ftest "${w} <= ${FOXS_W_MAX}"; do
                ./foxs -q "${max_q}" -w "${w}" -e "${e}" "${structure_file}" saxs.dat &> /dev/null
                output_name="$(printf '%s_%.2f_%.3f.dat' "${structure_base}" "${w}" "${e}")"
                mv "${structure_base}_saxs.dat" "${output_name}"
                w="$(feval "${w} + ${FOXS_W_STEP}")"
        done
        e="$(feval "${e} + ${FOXS_E_STEP}")"
done

# compress the results using parse-map
./parse-map "${structure_base}"

# copy out results
cp "${structure_base}.c12" "${request_workdir}" || exit 1

# clean things up
rm -rf "${SCRATCHDIR}/"*

exit 0
