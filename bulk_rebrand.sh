#!/bin/bash

# Cold or Hot run
RUN_TYPE="${1-cold}"

# Base directory (modify as needed)
BASE_DIR="$(pwd)"

# Log file
LOG_FILE="rebrand_log.log"

# Old & New Names
OLD_NAME="wazuh"
NEW_NAME="blackwell"

# Case mapping
CASE_MAP=(
    "wazuh:blackwell" 
    "Wazuh:Blackwell" 
    "WAZUH:BLACKWELL"
)

# Directories to ignore
IGNORE_DIRECTORIES=(
    ".git" 
    ".github"
)

# Files to ignore
IGNORE_FILES=(
    ".gitmodules"
    "bulk_rebrand.sh" 
    "bulk_rebrand.py" 
    "rebrand_log.log"
)

# File extensions to ignore (binaries)
IGNORE_EXTENSIONS=(
    "gz" 
    "tar" 
    "xz" 
    "zip" 
    "db" 
    "dll" 
    # "manifest" 
    # "exp" 
    "jpg" 
    "png" 
    # "log" 
    # "rtf"
    "pack" 
    "parquet" 
    "pem" 
    "wpk" 
    "tmp" 
    "repo" 
    "ico" 
    "pmc" 
    # "plist" 
    "lib" 
    "pmc"
)

# Number of files that had replaced content
FILES_MODIFIED=0
export FILES_MODIFIED
# Number of files that were renamed
FILES_REPLACED=0
export FILES_REPLACED
# Number of files were ignored
FILES_IGNORED=0
export FILES_IGNORED
# Number of directories that were ignored
DIRECTORIES_IGNORED=0
export DIRECTORIES_IGNORED

get_find_parameters() {
    local ignore_dirs=("${IGNORE_DIRECTORIES[@]}")
    local ignore_files=("${IGNORE_FILES[@]}")
    local ignore_extensions=("${IGNORE_EXTENSIONS[@]}")

    # Convert arrays to strings of -not -path flags
    local dir_flags=""
    for dir in "${ignore_dirs[@]}"; do
        if [[ -n "$dir_flags" ]]; then
            dir_flags+=" -o "
        fi
        dir_flags+="-path \"*/$dir/*\" -o -path \"*/$dir\" -o -path \"$dir/*\" -o -path \"$dir\" "
    done

    local file_flags=""
    for file in "${ignore_files[@]}"; do
        if [[ -n "$file_flags" ]]; then
            file_flags+=" -o "
        fi
        file_flags+="-name \"$file\" "
    done

    local ext_flags=""
    for ext in "${ignore_extensions[@]}"; do
        if [[ -n "$ext_flags" ]]; then
            ext_flags+=" -o "
        fi
        ext_flags+="-name \"*.$ext\" "
    done

    echo "-not \\( $dir_flags -o $file_flags -o $ext_flags \\)"
}

# 🔄 Renaming files & directories...
rename_file() {
    local path="$1"
    local file="$2"
    new_file="${file//WAZUH/BLACKWELL}"
    new_file="${new_file//Wazuh/Blackwell}"
    new_file="${new_file//wazuh/blackwell}"
    if [[ "${file}" != "${new_file}" ]]; then
        if [[ "${RUN_TYPE}" == "hot" ]]; then
            mv "${path}/${file}" "${path}/${new_file}"    
        fi
        echo "[RENAMED] ${path}/${file} -> ${path}/${new_file}" >> ${LOG_FILE}
    fi
    FILES_MODIFIED=$((FILES_MODIFIED+1))
}

# 🔄 Replacing content inside files...
replace_in_file() {
    local file="$1"
    if [[ "${RUN_TYPE}" == "hot" ]]; then
        tmp_file="${file}.tmp"
        awk '{
            gsub(/wazuh/, "blackwell");
            gsub(/Wazuh/, "Blackwell");
            gsub(/WAZUH/, "BLACKWELL");
            print
        }' "${file}" > "${tmp_file}" && mv "${tmp_file}" "${file}" 
    fi
    echo "[REPLACED] ${file}" >> ${LOG_FILE}
    FILES_REPLACED=$((FILES_REPLACED+1))
}

# 🔄 Replacing content inside tar files...
search_and_replace_in_tar_file() {
    local path="$1"
    local find_parameters="$2"
    local base=$(basename "${path}")
    local dir=$(dirname "${path}/")
    local tmpdir="${dir}/temp_${base}"

    mkdir "${tmpdir}"
    tar -xf "${path}" -C "${tmpdir}"
    search_and_replace_multiple_files "${tmpdir}" "${find_parameters}"
    tar -cf "${path}" -C "${tmpdir}" .
    rm -rf "${tmpdir}"
}

# 🔄 Replacing content on multiple files in a given directory...
search_and_replace_multiple_files() {
    local base_dir="$1"
    local find_parameters="$2"
    eval "find \"${base_dir}\" ${find_parameters}" | tac | while read -r path; do
        base=$(basename "${path}")
        dir=$(dirname "${path}/")
        if [ -f "${path}" ]; then
            replace_in_file "${path}"
        fi
        if echo ${base} | grep -i ${OLD_NAME} > /dev/null 2>&1; then
            rename_file "${DIR}" "${base}"
        fi
    done
}

# **STEP 1: Replace content inside files using awk**
if [[ "${RUN_TYPE}" == "hot" ]]; then
    echo "HOT run enabled. The changes below are going to be applied"
    echo "HOT run enabled. The changes below are going to be applied" > ${LOG_FILE}
elif [[ "${RUN_TYPE}" == "cold" ]]; then
    echo "COLD run enabled. The changes to be done are going to be listed bellow without being applied"
    echo "COLD run enabled. The changes to be done are going to be listed bellow without being applied" > ${LOG_FILE}
else
    echo "WARN: Neither COLD not HOT run were specified. Assuming COLD run was enabled. The changes to be done are going to be listed bellow without being applied"
    echo "COLD run enabled. The changes to be done are going to be listed bellow without being applied" > ${LOG_FILE}
    RUN_TYPE="cold"
fi

echo "Please wait while the script is running..."
echo "Handling plaintext files..."

FIND_PARAMETERS=$(get_find_parameters)
echo "find "${BASE_DIR}" ${FIND_PARAMETERS}"

search_and_replace_multiple_files "${BASE_DIR}" "${FIND_PARAMETERS}"

echo "Handling special format files"
find "${BASE_DIR}" -name "*.tar" | while read -r path; do
    search_and_replace_in_tar_file "${path}" "${FIND_PARAMETERS}"
done

echo "✅ Renaming done."

# **Summary**
echo "🎯 Rebranding complete. Run grep to verify remaining instances."
echo "Total of replaces in file:    ${FILES_MODIFIED}"
echo "Total of files replaced:      ${FILES_REPLACED}"
echo "Total of files ignored:       ${FILES_IGNORED}"
echo "Total of directories ignored: ${DIRECTORIES_IGNORED}"
echo "For the detailed list of affected files please check the rebrand_log.txt"
