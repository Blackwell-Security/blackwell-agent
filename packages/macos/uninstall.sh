#!/bin/sh

## Stop and remove application
sudo /Library/Ossec/bin/blackwell-control stop
sudo /bin/rm -r /Library/Ossec*

# remove launchdaemons
/bin/rm -f /Library/LaunchDaemons/com.blackwell.agent.plist

## remove StartupItems
/bin/rm -rf /Library/StartupItems/BLACKWELL

## Remove User and Groups
/usr/bin/dscl . -delete "/Users/blackwell"
/usr/bin/dscl . -delete "/Groups/blackwell"

/usr/sbin/pkgutil --forget com.blackwell.pkg.blackwell-agent
/usr/sbin/pkgutil --forget com.blackwell.pkg.blackwell-agent-etc

# In case it was installed via Puppet pkgdmg provider

if [ -e /var/db/.puppet_pkgdmg_installed_blackwell-agent ]; then
    rm -f /var/db/.puppet_pkgdmg_installed_blackwell-agent
fi

echo
echo "Blackwell agent correctly removed from the system."
echo

exit 0
