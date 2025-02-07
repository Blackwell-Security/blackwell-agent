#!/bin/bash

# Base directory (modify as needed)
BASE_DIR="$(pwd)"

# Cold or Hot run
RUN_TYPE="${$1:=cold}"

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

# Function to check if a path should be ignored
should_ignore() {
    local path="${1}"
    for dir in "${IGNORE_DIRECTORIES[@]}"; do
        if [[ "${path}" == "${dir}/"* ]]; then
            return 0 # Ignore this path
        elif [[ "${path}" == *"/${dir}/"* ]]; then
            return 0 # Ignore this path
        elif [[ "${path}" == *"/${dir}" ]]; then
            return 0 # Ignore this path
        fi
    done
    for file in "${IGNORE_FILES[@]}"; do
        if [[ "$(basename "${path}")" == "${file}" ]]; then
            return 0 # Ignore this file
        fi
    done
    for ext in "${IGNORE_EXTENSIONS[@]}"; do
        if [[ "${path}" == *".${ext}" ]]; then
            return 0 # Ignore this extension
        fi
    done
    return 1 # Do not ignore
}

# 🔄 Renaming files & directories...
rename_path() {
    local path="${1}"
    new_path="${path//WAZUH/BLACKWELL}"
    new_path="${new_path//Wazuh/Blackwell}"
    new_path="${new_path//wazuh/blackwell}"
    if [[ "${path}" != "${new_path}" ]]; then
        if [[ "${RUN_TYPE}" == "hot" ]]; then
            mv "${path}" "${new_path}"    
        fi
        echo "[RENAMED] ${path} -> ${new_path}" >> ${LOG_FILE}
    fi
}

# 🔄 Replacing content inside files...
replace_in_file()
{
    local file ="${1}"
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
}

# **STEP 1: Replace content inside files using awk**
if [[ "${RUN_TYPE}" == "hot" ]]; then
    echo "HOT run enabled. The changes below are going to be applied" > ${LOG_FILE}
elif [[ "${RUN_TYPE}" == "cold" ]]; then
    echo "COLD run enabled. The changes to be done are going to be listed bellow without being applied" > ${LOG_FILE}
else
    echo "ERROR: Neither COLD not HOT run were specified. Assuming COLD run was enabled"
    echo "HOT run enabled. The changes bellow are going to be appliet" > ${LOG_FILE}
    RUN_TYPE="cold"
fi

find "${BASE_DIR}" -depth -type f -o -type d | tail -r | while read -r path; do
    if should_ignore "${path}"; then
        continue
        echo "[IGNORED] ${path}" >> ${LOG_FILE}
    fi

    if [ -f "${path}" ]; then
        replace_in_file "${path}"
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