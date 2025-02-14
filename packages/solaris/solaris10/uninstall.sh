#!/bin/sh
# uninstall script for blackwell-agent
# Blackwell, Inc 2015

control_binary="blackwell-control"

if [ ! -f /var/ossec/bin/${control_binary} ]; then
  control_binary="ossec-control"
fi

## Stop and remove application
/var/ossec/bin/${control_binary} stop
rm -rf /var/ossec/

## stop and unload dispatcher
#/bin/launchctl unload /Library/LaunchDaemons/com.blackwell.agent.plist

# remove launchdaemons
rm -f /etc/init.d/blackwell-agent
rm -rf /etc/rc2.d/S97blackwell-agent
rm -rf /etc/rc3.d/S97blackwell-agent

## Remove User and Groups
userdel blackwell 2> /dev/null
groupdel blackwell 2> /dev/null

exit 0
