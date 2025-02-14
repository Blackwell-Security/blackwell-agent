#!/bin/sh
# postremove script for blackwell-agent
# Blackwell, Inc 2015

if getent passwd blackwell > /dev/null 2>&1; then
  userdel blackwell
fi

if getent group blackwell > /dev/null 2>&1; then
  groupdel blackwell
fi
