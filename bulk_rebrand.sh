#!/bin/bash

# Base directory (modify as needed)
BASE_DIR="$(pwd)"

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
    local path="$1"
    for dir in "${IGNORE_DIRECTORIES[@]}"; do
        if [[ "$path" == *"/$dir/"* ]]; then
            return 0 # Ignore this path
        fi
    done
    for file in "${IGNORE_FILES[@]}"; do
        if [[ "$(basename "$path")" == "$file" ]]; then
            return 0 # Ignore this file
        fi
    done
    for ext in "${IGNORE_EXTENSIONS[@]}"; do
        if [[ "$path" == *".$ext" ]]; then
            return 0 # Ignore this extension
        fi
    done
    return 1 # Do not ignore
}

# **STEP 1: Replace content inside files using awk**
echo "🔄 Replacing content inside files..."
find "$BASE_DIR" -type f | while read -r file; do
    if should_ignore "$file"; then
        continue
    fi

    # Use awk for case-sensitive replacements
    tmp_file="${file}.tmp"
    awk '{
        gsub(/\bwazuh\b/, "blackwell");
        gsub(/\bWazuh\b/, "Blackwell");
        gsub(/\bWAZUH\b/, "BLACKWELL");
        print
    }' "$file" > "$tmp_file" && mv "$tmp_file" "$file"

done
echo "✅ Content replacement done."

# **STEP 2: Rename files & directories (deepest first)**
echo "🔄 Renaming files & directories..."
find "$BASE_DIR" -depth -type f -o -type d | tail -r | while read -r path; do
    if should_ignore "$path"; then
        continue
    fi
    new_path="${path//WAZUH/BLACKWELL}"
    new_path="${new_path//Wazuh/Blackwell}"
    new_path="${new_path//wazuh/blackwell}"
    if [[ "$path" != "$new_path" ]]; then
        mv "$path" "$new_path"
        echo "[RENAMED] $path -> $new_path"
    fi
done
echo "✅ Renaming done."

# **Summary**
echo "🎯 Rebranding complete. Run grep to verify remaining instances."