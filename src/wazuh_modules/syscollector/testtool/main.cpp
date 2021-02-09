/*
 * Wazuh SysCollector Test tool
 * Copyright (C) 2015-2020, Wazuh Inc.
 * October 7, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <memory>
#include <chrono>
#include "defs.h"
#include "dbsync.h"
#include "rsync.h"
#include "sysInfo.hpp"
#include "syscollector.hpp"

constexpr auto DEFAULT_SLEEP_TIME { 60 };

static void logFunction(const char* msg)
{
    std::cout << msg << std::endl;
}

int main(int argc, const char* argv[])
{
    auto timedMainLoop { false };
    auto sleepTime { DEFAULT_SLEEP_TIME };
    if (2 == argc)
    {
        timedMainLoop = true;
        std::string firstArgument { argv[1] };

        sleepTime = firstArgument.find_first_not_of("0123456789") == std::string::npos ? std::stoi(firstArgument) : DEFAULT_SLEEP_TIME;

    }
    else if (2 < argc)
    {
        return -1;
    }

    const auto reportDiffFunction
    {
        [](const std::string& payload)
        {
            std::cout << "diff output payload:" << std::endl;
            std::cout << payload << std::endl;
        }
    };
    const auto reportSyncFunction
    {
        [](const std::string& payload)
        {
            std::cout << "sync output payload:" << std::endl;
            std::cout << payload << std::endl;
        }
    };

    const auto errorLogFunction
    {
        [](const std::string& log)
        {
            std::cout << "Error Log:" << std::endl;
            std::cout << log << std::endl;
        }
    };
    const auto infoLogFunction
    {
        [](const std::string& log)
        {
            std::cout << "Info Log:" << std::endl;
            std::cout << log << std::endl;
        }
    };
    const auto debugLogFunction
    {
        [](const std::string& log)
        {
            std::cout << "Debug Log:" << std::endl;
            std::cout << log << std::endl;
        }
    };
    const std::chrono::milliseconds timeout{5000};
    const auto spInfo{ std::make_shared<SysInfo>() };
    rsync_initialize(logFunction);
    dbsync_initialize(logFunction);
    try
    {
        std::thread thread
        {
            [timedMainLoop, sleepTime]
            {
                if (!timedMainLoop)
                {
                    while(std::cin.get() != 'q');
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::seconds(sleepTime));
                }
                Syscollector::instance().destroy();
            }
        };

        Syscollector::instance().init(spInfo,
                                      reportDiffFunction,
                                      reportSyncFunction,
                                      errorLogFunction,
                                      infoLogFunction,
                                      debugLogFunction,
                                      SYSCOLLECTOR_DB_DISK_PATH,
                                      SYSCOLLECTOR_NORM_CONFIG_DISK_PATH,
                                      SYSCOLLECTOR_NORM_TYPE,
                                      15ul,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true);

        if (thread.joinable())
        {
            thread.join();
        }
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    rsync_teardown();
    dbsync_teardown();
    return 0;
}