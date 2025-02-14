#!/bin/sh
# preremove script for blackwell-agent
# Blackwell, Inc 2015

control_binary="blackwell-control"

if [ ! -f /var/ossec/bin/${control_binary} ]; then
  control_binary="ossec-control"
fi

/var/ossec/bin/${control_binary} stop
