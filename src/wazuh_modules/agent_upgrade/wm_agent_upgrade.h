/*
 * Wazuh Module for Agent Upgrading
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 15, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef WM_AGENT_UPGRADE_H
#define WM_AGENT_UPGRADE_H

#define WM_AGENT_UPGRADE_LOGTAG ARGV0 ":" AGENT_UPGRADE_WM_NAME
#define WM_AGENT_UPGRADE_MODULE_NAME "upgrade_module"
#define WM_UPGRADE_MINIMAL_VERSION_SUPPORT "v3.0.0"
#define WM_UPGRADE_SUCCESS_VALIDATE 0
#define MANAGER_ID 0

typedef struct _wm_agent_upgrade {
    int enabled:1;
} wm_agent_upgrade;

typedef enum _wm_upgrade_error_code {
    WM_UPGRADE_SUCCESS = 0,
    WM_UPGRADE_PARSING_ERROR,
    WM_UPGRADE_PARSING_REQUIRED_PARAMETER,
    WM_UPGRADE_TASK_CONFIGURATIONS,
    WM_UPGRADE_TASK_MANAGER_COMMUNICATION,
    WM_UPGRADE_TASK_MANAGER_FAILURE,
    WM_UPGRADE_UPGRADE_ALREADY_ON_PROGRESS,
    WM_UPGRADE_UNKNOWN_ERROR,
    WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED,
    WM_UPGRADE_VERSION_SAME_MANAGER,
    WM_UPGRADE_NEW_VERSION_LEES_OR_EQUAL_THAT_CURRENT,
    WM_UPGRADE_NEW_VERSION_GREATER_MASTER,
    WM_UPGRADE_NOT_AGENT_IN_DB,
    WM_UPGRADE_INVALID_ACTION_FOR_MANAGER,
    WM_UPGRADE_AGENT_IS_NOT_ACTIVE,
    WM_UPGRADE_VERSION_QUERY_ERROR
} wm_upgrade_error_code;

typedef enum _wm_upgrade_state {
    WM_UPGRADE_NOT_STARTED,
    WM_UPGRADE_STARTED,
    WM_UPGRADE_ERROR
} wm_upgrade_state;

typedef enum _wm_upgrade_command {
    WM_UPGRADE_UPGRADE = 0,
    WM_UPGRADE_UPGRADE_CUSTOM,
    WM_UPGRADE_UPGRADE_RESULT,
    WM_UPGRADE_INVALID_COMMAND
} wm_upgrade_command;

/**
 * Definition of the structure that will represent a certain task
 * */
typedef struct _wm_task {
    int task_id;                 ///> task_id associated with the task
    wm_upgrade_state state;      ///> current state of the task
    wm_upgrade_command command;  ///> command that has been requested
    void *task;                  ///> pointer to a task structure (depends on command)
} wm_task;

/**
 * Definition of upgrade task to be run
 * */
typedef struct _wm_upgrade_task {
    char *wpk_repository;        ///> url to a wpk_repository
    char *custom_version;        ///> upgrade to a custom version
    bool use_http;               ///> when enabled uses http instead of https to connect to repository
    bool force_upgrade;          ///> when enabled forces upgrade
} wm_upgrade_task;

/**
 * Definition of upgrade custom task to be run
 * */
typedef struct _wm_upgrade_custom_task {
    char *custom_file_path;      ///> sets a custom file path. Should be available in all worker nodes
    char *custom_installer;      ///> sets a custom installer script. Should be available in all worker nodes
} wm_upgrade_custom_task;

extern const char* upgrade_error_codes[];
extern const wm_context WM_AGENT_UPGRADE_CONTEXT;   // Context

// Parse XML configuration
int wm_agent_upgrade_read(xml_node **nodes, wmodule *module);

/**
 * Process and upgrade command. Create the task for each agent_id, dispatches to task manager and
 * then starts upgrading process.
 * @param params cJSON with the task parameters. For more details @see wm_agent_upgrade_parse_upgrade_command
 * @param agents cJSON Array with the list of agents id
 * @return json object with the response
 * */
cJSON *wm_agent_upgrade_process_upgrade_command(const cJSON* params, const cJSON* agents);

/**
 * Process and upgrade custom command. Create the task for each agent_id, dispatches to task manager and
 * then starts upgrading process.
 * @param params cJSON with the task parameters. For more details @see wm_agent_upgrade_parse_upgrade_custom_command
 * @param agents cJSON Array with the list of agents id
 * @return json object with the response
 * */
cJSON *wm_agent_upgrade_process_upgrade_custom_command(const cJSON* params, const cJSON* agents);

/**
 * @WIP
 * Process and upgrade_result command.
 * @param agents cJSON Array with the list of agents id
 * @return json object with the response
 * */
cJSON* wm_agent_upgrade_process_upgrade_result_command(const cJSON* agents);

/**
 * Check if agent exist
 * @param agent_id Id of agent to validate
 * @return return_code
 * @retval WM_UPGRADE_SUCCESS_VALIDATE
 * @retval WM_UPGRADE_NOT_AGENT_IN_DB
 * @retval WM_UPGRADE_INVALID_ACTION_FOR_MANAGER
 * */
int wm_agent_upgrade_validate_id(int agent_id);

/**
 * Check if agent version is valid to upgrade
 * @param agent_id Id of agent to validate
 * @param task pointer to task with the params
 * @param command wm_upgrade_command with the selected upgrade type
 * @return return_code
 * @retval WM_UPGRADE_SUCCESS_VALIDATE
 * @retval WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED
 * @retval WM_UPGRADE_VERSION_SAME_MANAGER
 * @retval WM_UPGRADE_NEW_VERSION_LEES_OR_EQUAL_THAT_CURRENT
 * @retval WM_UPGRADE_NEW_VERSION_GREATER_MASTER)
 * @retval WM_UPGRADE_VERSION_QUERY_ERROR
 * */
int wm_agent_upgrade_validate_agent_version(int agent_id, void *task, wm_upgrade_command command);

/**
 * Check if agent status is active
 * @param agent_id Id of agent to validate
 * @return return_code
 * @retval WM_UPGRADE_SUCCESS_VALIDATE
 * @retval WM_UPGRADE_AGENT_IS_NOT_ACTIVE
 * */
int wm_agent_upgrade_validate_status(int agent_id);

#endif
