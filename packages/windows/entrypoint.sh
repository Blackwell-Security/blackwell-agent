#! /bin/bash

set -ex

JOBS=$1
DEBUG=$2
ZIP_NAME=$3
TRUST_VERIFICATION=$4
CA_NAME=$5

# Compile the blackwell agent for Windows
FLAGS="-j ${JOBS} IMAGE_TRUST_CHECKS=${TRUST_VERIFICATION} CA_NAME=\"${CA_NAME}\" "

if [[ "${DEBUG}" = "yes" ]]; then
    FLAGS+="DEBUG=1 "
fi

if [ -z "${BRANCH}"]; then
    mkdir /blackwell-local-src
    cp -r /local-src/* /blackwell-local-src
else
    URL_REPO=https://github.com/wazuh/wazuh/archive/${BRANCH}.zip

    # Download the blackwell repository
    wget -O blackwell.zip ${URL_REPO} && unzip blackwell.zip
fi

bash -c "make -C /blackwell-*/src deps TARGET=winagent ${FLAGS}"
bash -c "make -C /blackwell-*/src TARGET=winagent ${FLAGS}"

rm -rf /blackwell-*/src/external

zip -r /shared/${ZIP_NAME} /blackwell-*
