import os
import re
import shutil

# Base directory of the Blackwell Agent codebase (modify as needed)
BASE_DIR = os.getcwd()

# Old & New Names
OLD_NAME = "wazuh"
NEW_NAME = "blackwell"

# Preserve case variations
CASE_MAP = {
    "wazuh": "blackwell",
    "Wazuh": "Blackwell",
    "WAZUH": "BLACKWELL"
}

# Log file
LOG_FILE = "rebrand_log.txt"

# Directories to ignore (Git-safe)
IGNORE_DIRECTORIES = {
    ".git",
    ".github",
}

# Files to ignore (full filenames, regardless of extension)
IGNORE_FILES = {
    "bulk_rebrand.py",
    ".gitmodules",
}

# File extensions to ignore (binaries)
IGNORE_EXTENSIONS = {
    # Compressed files
    ".gz", ".tar", ".xz", ".zip",
    # Databases (likely binary)
    ".db",
    # Windows dynamic link libraries (binary)
    ".dll",
    # Binary/metadata for Windows
    ".manifest",
    # Export files from Windows builds
    ".exp",
    # Images (binary)
    ".jpg", ".png",
    # Libraries (binary)
    ".lib",
    # Logs (avoid corrupting logs)
    ".log",
    # Rich Text Format (binary)
    ".rtf",
    # Sample files (usually binary or irrelevant)
    ".sample",
    # Binary pack files (git, npm, etc.)
    ".pack", ".parquet", ".pem", ".wpk",
    # Output binary files
    ".out",
    # Temporary files
    ".tmp",
    # YUM, APT, etc. binary package files
    ".repo",
    # SSL/TLS certificates (dangerous to modify)
    ".pem",
}

def should_ignore_path(path):
    """Check if the path is inside an ignored directory."""
    return any(f"{os.sep}{ignore_dir}{os.sep}" in f"{path}{os.sep}" for ignore_dir in IGNORE_DIRECTORIES)

def should_ignore_file(file_path):
    """Check if the file should be ignored based on its name or extension."""
    filename = os.path.basename(file_path)
    file_ext = os.path.splitext(filename)[1].lower()
    return filename in IGNORE_FILES or file_ext in IGNORE_EXTENSIONS

def case_sensitive_replace(text):
    """Replace 'wazuh' in different cases with corresponding 'blackwell'."""
    def replace_match(match):
        return CASE_MAP.get(match.group(0), match.group(0))
    return re.sub(r"\b(WAZUH|Wazuh|wazuh)\b", replace_match, text)

def replace_in_file(file_path):
    """Replace 'wazuh' with 'blackwell' while preserving case in text files."""
    try:
        if should_ignore_path(file_path) or should_ignore_file(file_path):
            return False  # Skip ignored files

        with open(file_path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()

        new_content = case_sensitive_replace(content)
        if new_content != content:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            return True
    except Exception as e:
        print(f"❌ Skipping {file_path} (Error: {e})")
    return False

def gather_paths_for_renaming(root_dir):
    """Collect all files & directories that need renaming, sorted by depth (deepest first)."""
    rename_targets = []

    for dirpath, dirnames, filenames in os.walk(root_dir, topdown=True):
        if should_ignore_path(dirpath):
            continue  # Skip ignored directories

        # Collect file rename targets
        for filename in filenames:
            if OLD_NAME in filename and filename not in IGNORE_FILES:
                old_path = os.path.join(dirpath, filename)
                new_filename = filename.replace("WAZUH", "BLACKWELL").replace("Wazuh", "Blackwell").replace("wazuh", "blackwell")
                new_path = os.path.join(dirpath, new_filename)
                rename_targets.append((old_path, new_path))

        # Collect directory rename targets
        for dirname in dirnames[:]:
            if OLD_NAME in dirname:
                old_path = os.path.join(dirpath, dirname)
                new_dirname = dirname.replace("WAZUH", "BLACKWELL").replace("Wazuh", "Blackwell").replace("wazuh", "blackwell")
                new_path = os.path.join(dirpath, new_dirname)
                rename_targets.append((old_path, new_path))

    # **Sort by depth (deepest paths first) to avoid parent-child rename conflicts**
    rename_targets.sort(key=lambda x: x[0].count(os.sep), reverse=True)

    return rename_targets

def rename_files_and_dirs(root_dir):
    """Rename files and directories in correct order (deepest first)."""
    rename_targets = gather_paths_for_renaming(root_dir)

    for old_path, new_path in rename_targets:
        try:
            shutil.move(old_path, new_path)
            with open(LOG_FILE, "a") as log:
                log.write(f"[RENAMED] {old_path} -> {new_path}\n")
        except Exception as e:
            print(f"❌ Failed to rename {old_path} -> {new_path}: {e}")

def main():
    """Main function to process all files in the codebase."""
    modified_files = 0

    # Clear log file
    with open(LOG_FILE, "w") as log:
        log.write("🔹 Blackwell Rebranding Log 🔹\n")

    # **FIRST PASS** - Modify File Contents BEFORE renaming
    for dirpath, _, filenames in os.walk(BASE_DIR):
        if should_ignore_path(dirpath):
            continue  # Skip ignored directories

        for filename in filenames:
            file_path = os.path.join(dirpath, filename)
            if should_ignore_file(file_path):
                continue  # Skip ignored files

            if replace_in_file(file_path):
                modified_files += 1
                with open(LOG_FILE, "a") as log:
                    log.write(f"[MODIFIED] {file_path}\n")

    # **SECOND PASS** - Rename Files & Directories
    rename_files_and_dirs(BASE_DIR)

    print(f"✅ Done! Modified {modified_files} files. Check '{LOG_FILE}' for details.")

if __name__ == "__main__":
    main()