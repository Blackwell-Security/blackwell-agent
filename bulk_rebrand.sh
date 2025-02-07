#!/bin/bash

# Cold or Hot run
RUN_TYPE="${1-cold}"

# Base directory (modify as needed)
BASE_DIR="$(pwd)"

# Log file
LOG_FILE="rebrand_log.txt"

# Old & New Names
OLD_NAME="wazuh"
NEW_NAME="blackwell"

# Case mapping
CASE_MAP=("wazuh:blackwell" "Wazuh:Blackwell" "WAZUH:BLACKWELL")

# Directories to ignore
IGNORE_DIRECTORIES=(".git" ".github")

# Files to ignore
IGNORE_FILES=("bulk_rebrand.sh" "bulk_rebrand.py" ".gitmodules")

# File extensions to ignore (binaries)
IGNORE_EXTENSIONS=("gz" "tar" "xz" "zip" "db" "dll" "manifest" "exp" "jpg" "png" "log" "rtf"
                   "pack" "parquet" "pem" "wpk" "tmp" "repo")

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

# Function to check if a path should be ignored
should_ignore() {
    local path="$1"
    for dir in "${IGNORE_DIRECTORIES[@]}"; do
        if [[ "${path}" == "${dir}/"* ]]; then
            DIRECTORIES_IGNORED=$((DIRECTORIES_IGNORED+1))
            return 0 # Ignore this path
        elif [[ "${path}" == *"/${dir}/"* ]]; then
            DIRECTORIES_IGNORED=$((DIRECTORIES_IGNORED+1))
            return 0 # Ignore this path
        elif [[ "${path}" == *"/${dir}" ]]; then
            DIRECTORIES_IGNORED=$((DIRECTORIES_IGNORED+1))
            return 0 # Ignore this path
        fi
    done
    for file in "${IGNORE_FILES[@]}"; do
        if [[ "$(basename "${path}")" == "${file}" ]]; then
            FILES_IGNORED=$((FILES_IGNORED+1))
            return 0 # Ignore this file
        fi
    done
    for ext in "${IGNORE_EXTENSIONS[@]}"; do
        if [[ "${path}" == *".${ext}" ]]; then
            FILES_IGNORED=$((FILES_IGNORED+1))
            return 0 # Ignore this extension
        fi
    done
    return 1 # Do not ignore
}

get_list_of_files() {

}

# 🔄 Renaming files & directories...
rename_path() {
    local path="$1"
    new_path="${path//WAZUH/BLACKWELL}"
    new_path="${new_path//Wazuh/Blackwell}"
    new_path="${new_path//wazuh/blackwell}"
    if [[ "${path}" != "${new_path}" ]]; then
        if [[ "${RUN_TYPE}" == "hot" ]]; then
            mv "${path}" "${new_path}"    
        fi
        echo "[RENAMED] ${path} -> ${new_path}" >> ${LOG_FILE}
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

find "${BASE_DIR}" | tac | while read -r path; do
    if should_ignore "${path}"; then
        echo "[IGNORED] ${path}" >> ${LOG_FILE}
        continue
    fi

    if [ -f "${path}" ]; then
        if grep -i ${OLD_NAME} ${path}; then 
            replace_in_file "${path}"
        fi
        rename_path "${path}"
    elif [ -d "/path/to/something" ]; then
        rename_path "${path}"
    else
        echo "[IGNORED] ${path} It's neither a file nor a directory." >> ${LOG_FILE}
    fi

done
echo "✅ Renaming done."

# **Summary**
echo "🎯 Rebranding complete. Run grep to verify remaining instances."
echo "Total of replaces in file:    ${FILES_MODIFIED}"
echo "Total of files replaced:      ${FILES_REPLACED}"
echo "Total of files ignored:       ${FILES_IGNORED}"
echo "Total of directories ignored: ${DIRECTORIES_IGNORED}"
echo "For the detailed list of affected files please check the rebrand_log.txt"
