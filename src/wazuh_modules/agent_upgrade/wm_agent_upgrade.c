/*
 * Wazuh Module for Agent Upgrading
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 3, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wazuh_modules/wmodules.h"
#include "wm_agent_upgrade_parsing.h"
#include "wm_agent_upgrade_tasks.h"
#include "os_net/os_net.h"

/**
 * Module main function. It won't return
 * */
static void* wm_agent_upgrade_main(wm_agent_upgrade* upgrade_config);    
static void wm_agent_upgrade_destroy(wm_agent_upgrade* upgrade_config);  
static cJSON *wm_agent_upgrade_dump(const wm_agent_upgrade* upgrade_config);

/**
 * Start listening loop, exits only on error 
 * @param socket to listen to
 * @param timeout_sec timeout in seconds
 * @return only on errors, socket will be closed
 * */
static void wm_agent_upgrade_listen_messages(int sock, int timeout_sec);

/* Context definition */
const wm_context WM_AGENT_UPGRADE_CONTEXT = {
    AGENT_UPGRADE_WM_NAME,
    (wm_routine)wm_agent_upgrade_main,
    (wm_routine)(void *)wm_agent_upgrade_destroy,
    (cJSON * (*)(const void *))wm_agent_upgrade_dump
};

const char* upgrade_error_codes[] = {
    [WM_UPGRADE_SUCCESS] = "Success",
    [WM_UPGRADE_PARSING_ERROR] = "Could not parse message JSON",
    [WM_UPGRADE_PARSING_REQUIRED_PARAMETER] = "Required parameters in json message where not found",
    [WM_UPGRADE_TASK_CONFIGURATIONS] = "Command not recognized",
    [WM_UPGRADE_TASK_MANAGER_COMMUNICATION] ="Could not create task id for upgrade task",
    [WM_UPGRADE_TASK_MANAGER_FAILURE] = "", // Data string will be provided by task manager
    [WM_UPGRADE_UPGRADE_ALREADY_ON_PROGRESS] = "Upgrade procedure could not start. Agent already upgrading",
    [WM_UPGRADE_UNKNOWN_ERROR] "Upgrade procedure could not start",
    [WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED] = "Remote upgrade is not available for this agent version.",
    [WM_UPGRADE_VERSION_SAME_MANAGER] = "Agent and manager have the same version. No need upgrade.",
    [WM_UPGRADE_NEW_VERSION_LEES_OR_EQUAL_THAT_CURRENT] = "Current agent version is greater or equal.",
    [WM_UPGRADE_NEW_VERSION_GREATER_MASTER] = "Upgrading an agent to a version higher than the manager requires the force flag.",
    [WM_UPGRADE_NOT_AGENT_IN_DB] = "Not agent id found in database.",
    [WM_UPGRADE_INVALID_ACTION_FOR_MANAGER] = "Action not available for Manager (agent 000)",
    [WM_UPGRADE_AGENT_IS_NOT_ACTIVE] = "Agent is not active.",
    [WM_UPGRADE_VERSION_QUERY_ERROR] = "Not agent version found in database."
};

void wm_agent_upgrade_listen_messages(int sock, int timeout_sec) {
    struct timeval timeout = { timeout_sec, 0 };

    while(1) {
        // listen - wait connection
        fd_set fdset;    
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);

        switch (select(sock + 1, &fdset, NULL, NULL, &timeout)) {
        case -1:
            if (errno != EINTR) {
                merror(WM_UPGRADE_SELECT_ERROR, strerror(errno));
                close(sock);
                return;
            }
            continue;
        case 0:
            continue;
        }

        //Accept 
        int peer;
        if (peer = accept(sock, NULL, NULL), peer < 0) {
            if (errno != EINTR) {
                merror(WM_UPGRADE_ACCEPT_ERROR, strerror(errno));
            }
            continue;
        }
        
        // Get request string
        char *buffer = NULL;
        cJSON* json_response = NULL;
        cJSON* params = NULL;
        cJSON* agents = NULL;
        int parsing_retval;
        os_calloc(OS_MAXSTR, sizeof(char), buffer);
        int length;
        switch (length = OS_RecvSecureTCP(peer, buffer, OS_MAXSTR), length) {
        case OS_SOCKTERR:
            mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_SOCKTERR_ERROR);
            break;
        case -1:
            mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_RECV_ERROR, strerror(errno));
            break;
        case 0:
            mtdebug1(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_EMPTY_MESSAGE);
            break;
        default:
            /* Correctly received message */
            mtdebug1(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_INCOMMING_MESSAGE, buffer);
            parsing_retval = wm_agent_upgrade_parse_command(&buffer[0], &json_response, &params, &agents);
            break;
        }

        if (json_response) {
            cJSON *command_response = NULL;
            char* message = NULL;
            switch (parsing_retval)
            {
                case WM_UPGRADE_UPGRADE:
                    command_response = wm_agent_upgrade_process_upgrade_command(params, agents);
                    message = cJSON_PrintUnformatted(command_response); 
                    cJSON_Delete(command_response);
                    break;
                case WM_UPGRADE_UPGRADE_CUSTOM:
                    command_response = wm_agent_upgrade_process_upgrade_custom_command(params, agents);
                    message = cJSON_PrintUnformatted(command_response);
                    cJSON_Delete(command_response);
                    break;
                case WM_UPGRADE_UPGRADE_RESULT:
                    command_response = wm_agent_upgrade_process_upgrade_result_command(agents);
                    message = cJSON_PrintUnformatted(command_response); 
                    cJSON_Delete(command_response);
                    break;
                default:
                    message = cJSON_PrintUnformatted(json_response);
                    break;
            }
            mtdebug1(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_RESPONSE_MESSAGE, message);
            OS_SendSecureTCP(peer, strlen(message), message);
            os_free(message);
            cJSON_Delete(json_response);
        }

        free(buffer);
        close(peer);
    }
}

void * wm_agent_upgrade_main(wm_agent_upgrade* upgrade_config) {

    // Check if module is enabled
    if (!upgrade_config->enabled) {
        mtinfo(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_MODULE_DISABLED);
        pthread_exit(NULL);
    }

    mtinfo(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_MODULE_STARTED);

    // Initialize task hashmap
    wm_agent_upgrade_init_task_map();

    int sock = OS_BindUnixDomain(WM_UPGRADE_SOCK_PATH, SOCK_STREAM, OS_MAXSTR);
    if (sock < 0) {
        merror(WM_UPGRADE_BIND_SOCK_ERROR, WM_UPGRADE_SOCK_PATH, strerror(errno));
        return NULL;
    }

    wm_agent_upgrade_listen_messages(sock, 5);
    return NULL;
}

void wm_agent_upgrade_destroy(wm_agent_upgrade* upgrade_config) {
    mtinfo(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_MODULE_FINISHED);
    os_free(upgrade_config);

    wm_agent_upgrade_destroy_task_map();
}

cJSON *wm_agent_upgrade_dump(const wm_agent_upgrade* upgrade_config){
    cJSON *root = cJSON_CreateObject();
    cJSON *wm_info = cJSON_CreateObject();

    if (upgrade_config->enabled) {
        cJSON_AddStringToObject(wm_info,"enabled","yes"); 
    } else { 
        cJSON_AddStringToObject(wm_info,"enabled","no");
    }
    cJSON_AddItemToObject(root,"agent-upgrade",wm_info);
    return root;
}
