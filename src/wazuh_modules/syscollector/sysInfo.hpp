/*
 * Wazuh SysInfo
 * Copyright (C) 2015-2020, Wazuh Inc.
 * October 8, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _SYS_INFO_HPP
#define _SYS_INFO_HPP

#include "json.hpp"

constexpr auto KByte{1024};

class SysInfo
{
public:
    SysInfo() = default;
    // LCOV_EXCL_START
    virtual ~SysInfo() = default;
    // LCOV_EXCL_STOP
    nlohmann::json hardware();
    nlohmann::json packages();
    nlohmann::json os();
    nlohmann::json processes();
    nlohmann::json networks();    
private:
    virtual std::string getSerialNumber() const;
    virtual std::string getCpuName() const;
    virtual int getCpuMHz() const;
    virtual int getCpuCores() const;
    virtual void getMemory(nlohmann::json& info) const;
    virtual nlohmann::json getPackages() const;
    virtual nlohmann::json getOsInfo() const;
    virtual nlohmann::json getProcessesInfo() const;
    virtual nlohmann::json getNetworks() const;    
};

#endif //_SYS_INFO_HPP