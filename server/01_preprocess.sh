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

# read request's config
request_config="${request_dir}/config.request"

request_param="${request_dir}/params.txt"

echo OPTIM_STEPS=$(extract_param $request_param calcSteps) >>${request_config}
echo OPTIM_SYNC_STEPS=$(extract_param $request_param stepsBetweenSync) >>${request_config}
echo OPTIM_PARAM_ALPHA=$(extract_param $request_param alpha) >>${request_config}
echo OPTIM_PARAM_BETA=$(extract_param $request_param beta) >>${request_config}
echo OPTIM_PARAM_GAMMA=$(extract_param $request_param gama) >>${request_config}

source "${request_config}"

workdir="${request_dir}/workdir"
mkdir "${workdir}"
mkdir "${workdir}/structures"

# copy SAXS data
saxs_data="${request_dir}/${SAXS_DATA}"
cp "${saxs_data}" "${workdir}/saxs.dat"

# extract NMR structures
structures_file="${request_dir}/${STRUCTURES_FILE}"
filetype="$(file -ib "${structures_file}" | sed 's/;.*//')"
case "${filetype}" in

	"application/zip")
		unzip -qj "${structures_file}" -d "${workdir}/structures"
	;;
	"application/gzip")
		tar -xzf "${structures_file}" --xform="s/\//_/" -C "${workdir}/structures"
	;;
	"application/x-bzip2")
		tar -xjf "${structures_file}" --xform="s/\//_/" -C "${workdir}/structures"
	;;
	"application/x-rar")
		unrar --extract-no-paths "${structures_file}" "${workdir}/structures" > /dev/null
	;;
	*)
		basefile="$(basename "${structures_file}")"
		extension="${basefile##*.}"
		if [ "${extension}" != "pdb" ] && [ "${extension}" != "PDB" ]; then
			exit_error "${RETURN_USER_ERROR}" "Unsupported structures archive type: ${filetype}"
		fi

		sed -n '/^MODEL/, /ENDMDL/ p' "${structures_file}" | csplit --quiet --elide-empty-files --prefix="${workdir}/structures/structure" --suffix-format="%04d.pdb"  - "/^MODEL/" "{*}"
	;;
esac

[ $? -eq 0 ] || exit_error "${RETURN_USER_ERROR}" "Failed to decompress structures file"

# delete all non-PDB files in the structures directory
for file in "${workdir}/structures/"*; do
	if ! [ -f "${file}" ] || ! [[ "${file}" = *.pdb || "${file}" = *.PDB ]]; then
		rm -rf "${file}"
	fi
done

# rename all structures to the scheme structureXXXX.pdb
index=1
tmpdir="$(mktemp -d)"
shopt -s nullglob
mv "${workdir}/structures/"* "${tmpdir}"
for file in "${tmpdir}/"*; do
	newfilename="$(printf "structure%04d.pdb" "${index}")"
	mv "${file}" "${workdir}/structures/${newfilename}"
	((index += 1))
done
rmdir "${tmpdir}"

# initialize results file
structures_count="$(find "${workdir}/structures/" -name '*.pdb' | wc -l)"
entries_count=$((structures_count + 3))
printf "%${entries_count}s" | sed "s/ /0\.0000 /g" | sed "s/$/\n/" > "${request_dir}/results"
update_results ${request_dir}/result.dat weights "`printf "%${entries_count}s" | sed "s/ /0\.0000, /g" | sed "s/$/\n/" `"
shopt -u nullglob

exit "${RETURN_OK}"
