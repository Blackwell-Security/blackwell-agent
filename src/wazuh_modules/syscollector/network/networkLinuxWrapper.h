/*
 * Wazuh SYSCOLLECTOR
 * Copyright (C) 2015-2020, Wazuh Inc.
 * October 24, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _NETWORK_LINUX_WRAPPER_H
#define _NETWORK_LINUX_WRAPPER_H

#include <ifaddrs.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include "inetworkWrapper.h"
#include "networkHelper.h"
#include "filesystemHelper.h"
#include "stringHelper.h"
#include "sharedDefs.h"

static const std::map<std::pair<int, int>, std::string> NETWORK_INTERFACE_TYPE =
{
    { std::make_pair(ARPHRD_ETHER, ARPHRD_ETHER),               "ethernet"          },
    { std::make_pair(ARPHRD_PRONET, ARPHRD_PRONET),             "token ring"        },
    { std::make_pair(ARPHRD_PPP, ARPHRD_PPP),                   "point-to-point"    },
    { std::make_pair(ARPHRD_ATM, ARPHRD_ATM),                   "ATM"               },
    { std::make_pair(ARPHRD_IEEE1394, ARPHRD_IEEE1394),         "firewire"          },
    { std::make_pair(ARPHRD_TUNNEL, ARPHRD_IRDA),               "tunnel"            },
    { std::make_pair(ARPHRD_FCPP, ARPHRD_FCFABRIC),             "fibrechannel"      },
    { std::make_pair(ARPHRD_IEEE802_TR, ARPHRD_IEEE802154_PHY), "wireless"          },
};

static const std::map<std::string, std::string> DHCP_STATUS =
{
    { "dhcp",                   "enabled"           },
    { "yes",                    "enabled"           },
    { "static",                 "disabled"          },
    { "none",                   "disabled"          },
    { "no",                     "disabled"          },
    { "manual",                 "disabled"          },
    { "bootp",                  "BOOTP"             },
};

namespace GatewayFileFields
{
    enum 
    {
        Iface,
        Destination,
        Gateway,
        Flags,
        RefCnt,
        Use,
        Metric,
        Mask,
        MTU,
        Window,
        IRTT,
        Size
    };
}

namespace DebianInterfaceConfig
{
    enum Config
    {
        Type,
        Name,
        Family,
        Method,
        Size
    };
}

namespace RHInterfaceConfig
{
    enum Config
    {
        Key,
        Value,
        Size
    };
}

class NetworkLinuxInterface final : public INetworkInterfaceWrapper
{
    ifaddrs* m_interfaceAddress;
    static std::string getNameInfo(const sockaddr* inputData, const socklen_t socketLen)
    {
        auto retVal { std::make_unique<char[]>(NI_MAXHOST) };
        if (inputData)
        {
            const auto result { getnameinfo(inputData,
                socketLen,
                retVal.get(), NI_MAXHOST,
                NULL, 0, NI_NUMERICHOST) };
            
            if (result != 0)
            {
                throw std::runtime_error
                {
                    "Cannot get socket address information, Code: " + result
                };
            }
        }
        return retVal.get();
    }
 
    static std::string getRedHatDHCPStatus(const std::vector<std::string>& fields)
    {
        std::string retVal { "unknown" };
        const auto value { fields.at(RHInterfaceConfig::Value) };

        const auto it { DHCP_STATUS.find(value) };

        if (DHCP_STATUS.end() != it)
        {
            retVal = it->second;
        }

        return retVal;
    }

    static std::string getDebianDHCPStatus(const std::string& family, const std::vector<std::string>& fields)
    {
        std::string retVal { "enabled" };
        if (fields.at(DebianInterfaceConfig::Family).compare(family) == 0)
        {
            const auto method { fields.at(DebianInterfaceConfig::Method) };

            const auto it { DHCP_STATUS.find(method) };

            if (DHCP_STATUS.end() != it)
            {
                retVal = it->second;
            }
        }
        return retVal;
    }

public:
    explicit NetworkLinuxInterface(ifaddrs* addrs)
    : m_interfaceAddress(addrs)
    { 
        if (!addrs)
        {
            throw std::runtime_error { "Nullptr instances of network interface" };
        }
    }

    std::string name() const override
    {
        return m_interfaceAddress->ifa_name ? m_interfaceAddress->ifa_name : "";
    }

    std::string adapter() const override
    {
        return "unknown";
    }

    int family() const override
    {
        return m_interfaceAddress->ifa_addr ? m_interfaceAddress->ifa_addr->sa_family : AF_UNSPEC;
    }

    std::string address() const override
    {
        return m_interfaceAddress->ifa_addr ? getNameInfo(m_interfaceAddress->ifa_addr, sizeof(struct sockaddr_in)) : "";
    }
    
    std::string netmask() const override
    {
        return m_interfaceAddress->ifa_netmask ? getNameInfo(m_interfaceAddress->ifa_netmask, sizeof(struct sockaddr_in)) : "";
    }

    std::string broadcast() const override
    {
        std::string retVal;
        if (m_interfaceAddress->ifa_ifu.ifu_broadaddr)
        {
            retVal = getNameInfo(m_interfaceAddress->ifa_ifu.ifu_broadaddr, sizeof(struct sockaddr_in));
        }
        else 
        {
            const auto netmask { this->netmask() };
            const auto address { this->address() };
            if (address.size() && netmask.size())
            {
                retVal = Utils::NetworkHelper::getBroadcast(address, netmask);
            }
        }
        return retVal;
    }

    std::string addressV6() const override
    {
        return m_interfaceAddress->ifa_addr ? Utils::splitIndex(getNameInfo(m_interfaceAddress->ifa_addr, sizeof(struct sockaddr_in6)), '%', 0) : "";
    }

    std::string netmaskV6() const override
    {
        return m_interfaceAddress->ifa_netmask ? getNameInfo(m_interfaceAddress->ifa_netmask, sizeof(struct sockaddr_in6)) : "";
    }

    std::string broadcastV6() const override
    {
        return m_interfaceAddress->ifa_ifu.ifu_broadaddr ? getNameInfo(m_interfaceAddress->ifa_ifu.ifu_broadaddr, sizeof(struct sockaddr_in6)) : "";
    }

    std::string gateway() const override
    {
        std::string retVal { "unknown" };
        auto fileData { Utils::getFileContent(std::string(WM_SYS_NET_DIR) + "route") };
        const auto ifName { this->name() };
        if (!fileData.empty())
        {
            auto lines { Utils::split(fileData, '\n') };
            for (auto& line : lines)
            {
                line = Utils::rightTrim(line);
                Utils::replaceAll(line, "\t", " ");
                Utils::replaceAll(line, "  ", " ");
                const auto fields { Utils::split(line, ' ') };

                if (GatewayFileFields::Size == fields.size() &&
                    fields.at(GatewayFileFields::Iface).compare(ifName) == 0)
                {
                    const auto address { static_cast<uint32_t>(std::stoi(fields.at(GatewayFileFields::Gateway), 0, 16)) };
                    if (address)
                    {
                        retVal = std::string(inet_ntoa({ address })) + "|" + fields.at(GatewayFileFields::Metric);
                        break;
                    }
                }
            }
        }
        return retVal;
    }

    std::string metrics() const override
    {
        return "unknown";
    }

    std::string metricsV6() const override
    {
        return "unknown";
    }

    std::string dhcp() const override
    {
        auto fileData { Utils::getFileContent(WM_SYS_IF_FILE) };
        std::string retVal { "unknown" };
        const auto family { this->family() };
        const auto ifName { this->name() };
        if (!fileData.empty())
        {
            const auto lines { Utils::split(fileData, '\n') };
            for (const auto& line : lines)
            {
                const auto fields { Utils::split(line, ' ') };
                if (DebianInterfaceConfig::Size == fields.size())
                {
                    if (fields.at(DebianInterfaceConfig::Type).compare("iface") == 0 &&
                        fields.at(DebianInterfaceConfig::Name).compare(ifName) == 0)
                    {
                        if (AF_INET == family)
                        {
                            retVal = getDebianDHCPStatus("inet", fields);
                            break;
                        }
                        else if (AF_INET6 == family)
                        {
                            retVal = getDebianDHCPStatus("inet6", fields);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            const auto fileName { "ifcfg-" + ifName };
            fileData = Utils::getFileContent(WM_SYS_IF_DIR_RH + fileName);
            fileData = fileData.empty() ? Utils::getFileContent(WM_SYS_IF_DIR_SUSE + fileName) : fileData;

            if (!fileData.empty())
            {
                const auto lines { Utils::split(fileData, '\n') };
                for (const auto& line : lines)
                {
                    const auto fields { Utils::split(line, '=') };
                    if (fields.size() == RHInterfaceConfig::Size)
                    {
                        if (AF_INET == family)
                        {
                            if (fields.at(RHInterfaceConfig::Key).compare("BOOTPROTO") == 0)
                            {
                                retVal = getRedHatDHCPStatus(fields);
                                break;
                            }
                        }
                        else if (AF_INET6 == family)
                        {
                            if (fields.at(RHInterfaceConfig::Key).compare("DHCPV6C") == 0)
                            {
                                retVal = getRedHatDHCPStatus(fields);
                                break;
                            }
                        }
                    }
                }
            }
        }
        return retVal;
    }

    std::string mtu() const override
    {
        std::string retVal;
        const auto name { this->name() };
        if (!name.empty())
        {
            const auto mtuFileContent { Utils::getFileContent(std::string(WM_SYS_IFDATA_DIR) + name + "/mtu") };
            retVal = Utils::splitIndex(mtuFileContent, '\n', 0);
        }
        return retVal;
    }

    LinkStats stats() const override
    {
        return m_interfaceAddress->ifa_data ? *reinterpret_cast<LinkStats *>(m_interfaceAddress->ifa_data) : LinkStats();
    }

    std::string type() const override
    {
        const auto networkTypeCode { Utils::getFileContent(std::string(WM_SYS_IFDATA_DIR) + this->name() + "/type") };
        return Utils::NetworkHelper::getNetworkTypeStringCode(std::stoi(networkTypeCode), NETWORK_INTERFACE_TYPE);
    }

    std::string state() const override
    {
        const auto operationalState { Utils::getFileContent(std::string(WM_SYS_IFDATA_DIR) + this->name() + "/operstate") };
        return Utils::splitIndex(operationalState, '\n', 0);
    }

    std::string MAC() const override
    {
        const auto mac { Utils::getFileContent(std::string(WM_SYS_IFDATA_DIR) + this->name() + "/address")};
        return Utils::splitIndex(mac, '\n', 0);
    }
};

#endif // _NETWORK_LINUX_WRAPPER_H