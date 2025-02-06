import os
import re

# Base directory of the Wazuh Agent codebase (modify as needed)
BASE_DIR = os.getcwd()  

# Keywords to replace
OLD_NAME = "wazuh"
NEW_NAME = "blackwell"

# Log file to track replacements
LOG_FILE = "rebrand_log.txt"

# File extensions to ignore (binaries, images, compressed files)
IGNORE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".gif", ".ico", ".exe", ".dll", ".so", ".zip", ".tar", ".gz", ".bin", "bulk_rebrand.py"}

# Directories to ignore (won't enter or modify anything inside)
IGNORE_DIRECTORIES = {".git", ".github", "node_modules", "venv", "__pycache__"}

def should_ignore_path(path):
    """Check if the path is inside an ignored directory."""
    for ignore_dir in IGNORE_DIRECTORIES:
        if f"{os.sep}{ignore_dir}{os.sep}" in f"{path}{os.sep}":  # Ensures full match, avoids partial matches
            return True
    return False

def replace_in_file(file_path):
    """Replace 'wazuh' with 'blackwell' in text files."""
    try:
        if should_ignore_path(file_path):
            return False  # Skip ignored directories

        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        
        new_content = re.sub(rf"\b{OLD_NAME}\b", NEW_NAME, content, flags=re.IGNORECASE)

        if new_content != content:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            return True
    except Exception as e:
        print(f"❌ Skipping {file_path} (Error: {e})")
    return False

def rename_files_and_dirs(root_dir):
    """Rename files and directories that contain 'wazuh', except ignored ones."""
    for dirpath, dirnames, filenames in os.walk(root_dir, topdown=False):
        if should_ignore_path(dirpath):
            continue  # Skip ignored directories

        for filename in filenames:
            if OLD_NAME in filename.lower():
                old_path = os.path.join(dirpath, filename)
                new_path = os.path.join(dirpath, filename.lower().replace(OLD_NAME, NEW_NAME))
                os.rename(old_path, new_path)
                with open(LOG_FILE, "a") as log:
                    log.write(f"[FILE RENAMED] {old_path} -> {new_path}\n")

        for dirname in dirnames[:]:  # Iterate over a copy of dirnames to modify in-place
            if OLD_NAME in dirname.lower():
                old_path = os.path.join(dirpath, dirname)
                new_path = os.path.join(dirpath, dirname.lower().replace(OLD_NAME, NEW_NAME))

                if should_ignore_path(old_path):
                    continue  # Skip ignored directories

                os.rename(old_path, new_path)
                with open(LOG_FILE, "a") as log:
                    log.write(f"[DIR RENAMED] {old_path} -> {new_path}\n")

def main():
    """Main function to process all files in the codebase."""
    modified_files = 0

    # Clear log file
    with open(LOG_FILE, "w") as log:
        log.write("🔹 Blackwell Rebranding Log 🔹\n")

    for dirpath, _, filenames in os.walk(BASE_DIR):
        if should_ignore_path(dirpath):
            continue  # Skip ignored directories

        for filename in filenames:
            file_path = os.path.join(dirpath, filename)
            file_ext = os.path.splitext(filename)[1].lower()

            if file_ext in IGNORE_EXTENSIONS:
                continue  # Skip binary files

            if replace_in_file(file_path):
                modified_files += 1
                with open(LOG_FILE, "a") as log:
                    log.write(f"[MODIFIED] {file_path}\n")

    # Rename files & directories
    rename_files_and_dirs(BASE_DIR)

    print(f"✅ Done! Modified {modified_files} files. Check '{LOG_FILE}' for details.")

if __name__ == "__main__":
    main()