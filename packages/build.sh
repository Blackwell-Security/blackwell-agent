#!/bin/bash

# Blackwell package builder
# Copyright (C) 2015, Blackwell Inc.
#
# This program is a free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public
# License (version 2) as published by the FSF - Free Software
# Foundation.
set -e

build_directories() {
  local build_folder=$1
  local blackwell_dir="$2"
  local future="$3"

  mkdir -p "${build_folder}"
  blackwell_version="$(cat blackwell*/src/VERSION| cut -d 'v' -f 2)"

  if [[ "$future" == "yes" ]]; then
    blackwell_version="$(future_version "$build_folder" "$blackwell_dir" $blackwell_version)"
    source_dir="${build_folder}/blackwell-${BUILD_TARGET}-${blackwell_version}"
  else
    package_name="blackwell-${BUILD_TARGET}-${blackwell_version}"
    source_dir="${build_folder}/${package_name}"
    cp -R $blackwell_dir "$source_dir"
  fi
  echo "$source_dir"
}

# Function to handle future version
future_version() {
  local build_folder="$1"
  local blackwell_dir="$2"
  local base_version="$3"

  specs_path="$(find $blackwell_dir -name SPECS|grep $SYSTEM)"

  local major=$(echo "$base_version" | cut -dv -f2 | cut -d. -f1)
  local minor=$(echo "$base_version" | cut -d. -f2)
  local version="${major}.30.0"
  local old_name="blackwell-${BUILD_TARGET}-${base_version}"
  local new_name=blackwell-${BUILD_TARGET}-${version}

  local new_blackwell_dir="${build_folder}/${new_name}"
  cp -R ${blackwell_dir} "$new_blackwell_dir"
  find "$new_blackwell_dir" "${specs_path}" \( -name "*VERSION*" -o -name "*changelog*" \
        -o -name "*.spec" \) -exec sed -i "s/${base_version}/${version}/g" {} \;
  sed -i "s/\$(VERSION)/${major}.${minor}/g" "$new_blackwell_dir/src/Makefile"
  sed -i "s/${base_version}/${version}/g" $new_blackwell_dir/src/init/blackwell-{server,client,local}.sh
  echo "$version"
}

# Function to generate checksum and move files
post_process() {
  local file_path="$1"
  local checksum_flag="$2"
  local source_flag="$3"

  if [[ "$checksum_flag" == "yes" ]]; then
    sha512sum "$file_path" > /var/local/checksum/$(basename "$file_path").sha512
  fi

  if [[ "$source_flag" == "yes" ]]; then
    mv "$file_path" /var/local/blackwell
  fi
}

# Main script body

# Script parameters
export REVISION="$1"
export JOBS="$2"
debug="$3"
checksum="$4"
future="$5"
legacy="$6"
src="$7"

build_dir="/build_blackwell"

source helper_function.sh

if [ -n "${BLACKWELL_VERBOSE}" ]; then
  set -x
fi

# Download source code if it is not shared from the local host
if [ ! -d "/blackwell-local-src" ] ; then
    curl -sL https://github.com/wazuh/wazuh/tarball/${WAZUH_BRANCH} | tar zx
    short_commit_hash="$(curl -s https://api.github.com/repos/blackwell/blackwell/commits/${BLACKWELL_BRANCH} \
                          | grep '"sha"' | head -n 1| cut -d '"' -f 4 | cut -c 1-11)"
else
    if [ "${legacy}" = "no" ]; then
      short_commit_hash="$(cd /blackwell-local-src && git rev-parse --short HEAD)"
    else
      # Git package is not available in the CentOS 5 repositories.
      head=$(cat /blackwell-local-src/.git/HEAD)
      hash_commit=$(echo "$head" | grep "ref: " >/dev/null && cat /blackwell-local-src/.git/$(echo $head | cut -d' ' -f2) || echo $head)
      short_commit_hash="$(cut -c 1-11 <<< $hash_commit)"
    fi
fi

# Build directories
source_dir=$(build_directories "$build_dir/${BUILD_TARGET}" "blackwell*" $future)

blackwell_version="$(cat $source_dir/src/VERSION| cut -d 'v' -f 2)"
# TODO: Improve how we handle package_name
# Changing the "-" to "_" between target and version breaks the convention for RPM or DEB packages.
# For now, I added extra code that fixes it.
package_name="blackwell-${BUILD_TARGET}-${blackwell_version}"
specs_path="$(find $source_dir -name SPECS|grep $SYSTEM)"

setup_build "$source_dir" "$specs_path" "$build_dir" "$package_name" "$debug"

set_debug $debug $sources_dir

# Installing build dependencies
cd $sources_dir
build_deps $legacy
build_package $package_name $debug "$short_commit_hash" "$blackwell_version"

# Post-processing
get_package_and_checksum $blackwell_version $short_commit_hash $src
