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
    "invalid_utf8.xml" # Ignore Edge Case UTF-16 file
    "rebrand_log.log"
)

# File extensions to ignore (binaries)
IGNORE_EXTENSIONS=(
    "gz"
    "tar"
    "xz"
    "zip"
    "dll"
    "jpg"
    "png"
    "pack"
    "parquet"
    "pem"
    "tmp"
    "repo"
    "ico"
    "pmc"
    "lib"
    "pmc"
)

# Make sure these dependencies are installed before running the script
check_dependencies(){
    local dependencies=(
        sqlite3
        plutil
    )

    local all_good="yes"
    for dep in "${dependencies[@]}"; do
        if ! command -v "$dep" > /dev/null; then
            echo "[ERROR] $dep is missing."
            echo "[ERROR] $dep is missing." >> ${LOG_FILE}
            all_good="no"
        fi
    done
    if [[ "${all_good}" == "no" ]]; then
        echo "[Error] Please install the missing dependencies and try again..."
        echo "[Error] Please install the missing dependencies and try again..." >> ${LOG_FILE}
        exit 1
    fi
}

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
}

# 🔄 Replacing content inside tar files...
search_and_replace_in_tar_file() {
    local path="$1"
    local find_parameters="$2"
    local base=$(basename "${path}")
    local dir=$(dirname "${path}/")
    local tmpdir="${dir}/temp_${base}"

    if [[ "${RUN_TYPE}" == "hot" ]]; then
        mkdir "${tmpdir}"
        tar -xf "${path}" -C "${tmpdir}"
        search_and_replace_multiple_files "${tmpdir}" "${find_parameters}"
        tar -cf "${path}" -C "${tmpdir}" .
        rm -rf "${tmpdir}"
    fi
}

# 🔄 Replacing content inside .db sqlite3 files...
search_and_replace_in_sqlite3_file(){
    local file="$1"
    local base=$(basename "${file}")
    local dir=$(dirname "${file}/")
    local tmpfile="${dir}/tmp_${base}.sql"

    if [[ "${RUN_TYPE}" == "hot" ]]; then
        # Created temp .sql file as plaintext
        sqlite3 "${file}" ".dump" > "${tmpfile}"
        # Dumped .db into tmpfile
        replace_in_file "${tmpfile}"

        # 🛑 **Delete the existing SQLite DB to avoid conflicts**
        rm -f "${file}"
        touch "${file}"
        sqlite3 "${file}" < "${tmpfile}"
        rm -f "${tmpfile}"
    fi
}

# 🔄 Replacing content inside .plist Apple binary files...
search_and_replace_in_plist_file(){
    local file="$1"
    
    if [[ "${RUN_TYPE}" == "hot" ]]; then
        plutil -convert xml1 "${file}"
        replace_in_file "${file}"
        plutil -convert binary1 "${file}"
        rm -f "${file}"
    fi
}

# 🔄 Replacing content on multiple files in a given directory...
search_and_replace_multiple_files() {
    local base_dir="$1"
    local find_parameters="$2"
    eval "find \"${base_dir}\" ${find_parameters}" | tac | while read -r path; do
        base=$(basename "${path}")
        dir=$(dirname "${path}/")
        if [ -f "${path}" ]; then
            if [[ "${path}" == *.db ]]; then
                if file "${path}" | grep "SQLite 3" > /dev/null ; then
                    search_and_replace_in_sqlite3_file "${path}"
                else
                    replace_in_file "${path}"
                fi
            elif [[ "${path}" == *.plist ]]; then
                if file "${path}" | grep "Apple binary property list" > /dev/null ; then
                    search_and_replace_in_plist_file "${path}"
                else
                    replace_in_file "${path}"
                fi
            else
                replace_in_file "${path}"
            fi
        fi
        if echo ${base} | grep -i ${OLD_NAME} > /dev/null 2>&1; then
            rename_file "${dir}" "${base}"
        fi
    done
}

replace_resource_url_base_in_makefile() {
    local makefile="${BASE_DIR}/$1"

    if [[ "${RUN_TYPE}" == "hot" ]]; then
        tmp_file="${makefile}.tmp"
        awk '{
            gsub(/packages.blackwell.com/, "packages.wazuh.com");
            print
        }' "${makefile}" > "${tmp_file}" && mv "${tmp_file}" "${makefile}" 
    fi
}

echo "Checking dependencies..."
check_dependencies
echo "Dependencies verified, continue..."

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
# Print find command for debug purposes to verify parameters
echo "[DEBUG] find "${BASE_DIR}" ${FIND_PARAMETERS}" >> "${LOG_FILE}"

# Rename directories
search_and_replace_multiple_files "${BASE_DIR}" "-type d ${FIND_PARAMETERS}"
# Replace in files
search_and_replace_multiple_files "${BASE_DIR}" "${FIND_PARAMETERS}"

echo "Handling special format files..."
find "${BASE_DIR}" -name "*.tar" | while read -r path; do
    search_and_replace_in_tar_file "${path}" "${FIND_PARAMETERS}"
done

# Replace packages.blackwell.com back to packages.wazuh.com so makefile pulls libraries from wazuh until we configure blackwell domain
echo "Handling src/Makefile edge cases..."
replace_resource_url_base_in_makefile "src/Makefile"

echo "✅ Renaming done."

# **Summary**
echo "🎯 Rebranding complete."
echo "For the detailed list of affected files please check the rebrand_log.txt"
