/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef SYSCHECK_H
#define SYSCHECK_H

#include "config/syscheck-config.h"
#include "syscheck_op.h"
#include "external/cJSON/cJSON.h"

#define MAX_LINE PATH_MAX+256

/* Notify list size */
#define NOTIFY_LIST_SIZE    32

#define WDATA_DEFAULT_INTERVAL_SCAN 300

#ifdef WIN32
#define FIM_REGULAR _S_IFREG
#define FIM_DIRECTORY _S_IFDIR
#else
#define FIM_REGULAR S_IFREG
#define FIM_DIRECTORY S_IFDIR
#define FIM_LINK S_IFLNK
#endif

/* Global config */
extern syscheck_config syscheck;
extern int sys_debug_level;

typedef enum fim_event_type {
    FIM_ADD,
    FIM_DELETE,
    FIM_MODIFICATION
} fim_event_type;

typedef enum fim_scan_event {
    FIM_SCAN_START,
    FIM_SCAN_END
} fim_scan_event;

typedef struct fim_element {
    struct stat statbuf;
    int index;
    int configuration;
    int mode;
} fim_element;

/* Win32 does not have lstat */
#ifdef WIN32
    #define w_stat(x, y) stat(x, y)
#else
    #define w_stat(x, y) lstat(x, y)
#endif

/** Function Prototypes **/

/* Check the integrity of the files against the saved database */
void run_check(void);

/* Run run_check periodically */
void start_daemon(void);

/* Read the XML config */
int Read_Syscheck_Config(const char *cfgfile) __attribute__((nonnull));

/* Parse read config into JSON format */
cJSON *getSyscheckConfig(void);
cJSON *getSyscheckInternalOptions(void);

// TODO: Add Doxygen description to functions
//
void read_internal(int debug_level);

// Create the database
void fim_scan();

//
void fim_checker(char * path, fim_element *item, whodata_evt *w_evt, int report);

//
int fim_directory (char * dir, fim_element *item, whodata_evt * w_evt, int report);

//
int fim_file (char * file, fim_element *item, whodata_evt * w_evt, int report);

//
void fim_realtime_event(char *file);

//
void fim_whodata_event(whodata_evt * w_evt);

//
void fim_audit_inode_event(char *file, const char *inode_key, fim_event_mode mode, whodata_evt * w_evt);

//
int fim_registry_event (char * key, fim_entry_data * data, int pos);

//
int fim_configuration_directory(const char *path, const char *entry);

//
int fim_check_depth(char * path, int dir_position);

//
fim_entry_data * fim_get_data (const char * file_name, fim_element *item);

//
void init_fim_data_entry(fim_entry_data *data);

//
void fim_get_checksum (fim_entry_data * data);

//
int fim_insert(char *file_name, fim_entry_data * data, struct stat *file_stat);

//
int fim_update (char * file, fim_entry_data * data, fim_entry_data * old_data);

#ifndef WIN32
//
int fim_update_inode(char * file, char inode_key[]);
#endif

//
void fim_delete(char *file_name);

//
void fim_print_info(struct timespec start, struct timespec end, clock_t cputime_start);

//
void print_rbtree();

//
void print_inodes();

//
void print_dirtb();

//
void check_deleted_files();

//
void delete_inode_item(char *inode, char *file_name);

/**
 * @brief Produce a file change JSON event
 *
 * {
 *   type:                  "event"
 *   data: {
 *     path:                string
 *     mode:                "scheduled"|"real-time"|"whodata"
 *     type:                "added"|"deleted"|"modified"
 *     timestamp:           number
 *     tags:                string
 *     content_changes:     string
 *     changed_attributes:  array   fim_json_compare_attrs()    [Only if old_data]
 *     old_attributes:      object  fim_attributes_json()       [Only if old_data]
 *     attributes:          object  fim_attributes_json()
 *     audit:               object  fim_audit_json()
 *   }
 * }
 *
 * @param file_name File path.
 * @param old_data Previous file state.
 * @param new_data Current file state.
 * @param dir_position Index of the related configuration stanza.
 * @param type Type of event: added, deleted or modified.
 * @param mode Event source.
 * @param w_evt Audit data structure.
 * @return File event JSON object.
 * @retval NULL No changes detected. Do not send an event.
 */
cJSON *fim_json_event(char *file_name, fim_entry_data *old_data, fim_entry_data *new_data, int pos, fim_event_type type, fim_event_mode mode, whodata_evt *w_evt);

//
void free_entry_data(fim_entry_data * data);

//
void free_inode_data(fim_inode_data ** data);

/* Check the registry for changes */
void os_winreg_check(void);

/* Start real time */
int realtime_start(void);

/* Add a directory to real time monitoring */
int realtime_adddir(const char *dir, int whodata) __attribute__((nonnull(1)));

/* Process real time queue */
void realtime_process(void);

/* Delete data form dir_tb hash table */
void free_syscheck_dirtb_data(char *data);

/* Process the content of the file changes */
char *seechanges_addfile(const char *filename) __attribute__((nonnull));

/* Generate the whodata csum */
void free_whodata_event(whodata_evt *w_evt);

void send_syscheck_msg(const char *msg) __attribute__((nonnull));
void fim_send_sync_msg(const char * msg);
int send_log_msg(const char * msg);

#ifdef __linux__
#define READING_MODE 0
#define HEALTHCHECK_MODE 1
int audit_init(void);
char * audit_get_id(const char * event);
int init_regex(void);
int add_audit_rules_syscheck(bool first_time);
void audit_read_events(int *audit_sock, int reading_mode);
void audit_set_db_consistency(void);
int check_auditd_enabled(void);
int set_auditd_config(void);
int init_auditd_socket(void);
void *audit_main(int *audit_sock);
void *audit_reload_thread();
void *audit_healthcheck_thread(int *audit_sock);
char *gen_audit_path(char *cwd, char *path0, char *path1);
void audit_reload_rules(void);
void audit_parse(char *buffer);
int audit_health_check(int audit_socket);
void clean_rules(void);
int filterkey_audit_events(char *buffer);
extern W_Vector *audit_added_dirs;
extern volatile int audit_thread_active;
extern volatile int whodata_alerts;
extern volatile int audit_db_consistency_flag;
extern pthread_mutex_t audit_mutex;
extern pthread_cond_t audit_thread_started;
extern pthread_cond_t audit_hc_started;
extern pthread_cond_t audit_db_consistency;
#elif WIN32
/* Initializes the whodata scan mode */
int run_whodata_scan(void);
int whodata_audit_start();
int set_winsacl(const char *dir, int position);
long unsigned int WINAPI state_checker(__attribute__((unused)) void *_void);
#endif

int is_nodiff(const char *filename);
void fim_initialize();
int fim_whodata_initialize();

/* Check for restricts and ignored files */
int fim_check_ignore(const char *file_name);
int fim_check_restrict(const char *file_name, OSMatch *restriction);

#ifndef WIN32
// Com request thread dispatcher
void * syscom_main(void * arg);
#endif
size_t syscom_dispatch(char *command, char ** output);
size_t syscom_getconfig(const char * section, char ** output);

#ifdef WIN_WHODATA
int w_update_sacl(const char *obj_path);
#endif

#ifdef WIN32
#define check_removed_file(x) ({ strstr(x, ":\\$recycle.bin") ? 1 : 0; })
#endif

void * fim_run_integrity(void * args);

void fim_sync_checksum();
void fim_sync_checksum_split(const char * start, const char * top, long id);
void fim_sync_send_list(const char * start, const char * top);
void fim_sync_dispatch(char * payload);
void fim_sync_push_msg(const char * msg);

/**
 * @brief Create file attribute set JSON from a FIM entry structure
 *
 * Format:
 * {
 *   type:        "file"|"registry"
 *   size:        number
 *   perm:        string
 *   user_name:   string
 *   group_name:  string
 *   uid:         string
 *   gid:         string
 *   inode:       number
 *   mtime:       number
 *   hash_md5:    string
 *   hash_sha1:   string
 *   hash_sha256: string
 *   checksum:    string
 * }
 *
 * @param data Pointer to a FIM entry structure.
 * @pre data is mutex-blocked.
 * @return Pointer to cJSON structure.
 */
cJSON * fim_attributes_json(const fim_entry_data * data);

/**
 * @brief Create file entry JSON from a FIM entry structure
 *
 * Format:
 * {
 *   path:              string
 *   timestamp:         number
 *   attributes: {
 *     type:            "file"|"registry"
 *     size:            number
 *     perm:            string
 *     user_name:       string
 *     group_name:      string
 *     uid:             string
 *     gid:             string
 *     inode:           number
 *     mtime:           number
 *     hash_md5:        string
 *     hash_sha1:       string
 *     hash_sha256:     string
 *     win_attributes:  string
 *     symlink_path:    string
 *     checksum:        string
 *   }
 * }
 *
 * @param path Pointer to file path string.
 * @param data Pointer to a FIM entry structure.
 * @pre data is mutex-blocked.
 * @return Pointer to cJSON structure.
 */
cJSON * fim_entry_json(const char * path, fim_entry_data * data);

/**
 * @brief Create file attribute comparison JSON object
 *
 * Format: array of strings, with the following possible strings:
 * - size
 * - permission
 * - uid
 * - user_name
 * - gid
 * - group_name
 * - mtime
 * - inode (UNIX only)
 * - md5
 * - sha1
 * - sha256
 *
 * @param old_data
 * @param new_data
 * @return cJSON*
 */
cJSON * fim_json_compare_attrs(const fim_entry_data * old_data, const fim_entry_data * new_data);

/**
 * @brief Create file audit data JSON object
 *
 * Format:
 * {
 *   user_id:        string
 *   user_name:      string
 *   group_id:       string
 *   group_name:     string
 *   process_name:   string
 *   audit_uid:      string
 *   audit_name:     string
 *   effective_uid:  string
 *   effective_name: string
 *   ppid:           number
 *   process_id:     number
 * }
 *
 * @param w_evt Pointer to event whodata structure
 * @return cJSON object pointer.
 */
cJSON * fim_audit_json(const whodata_evt * w_evt);

/**
 * @brief Create scan info JSON event
 *
 * Format:
 * {
 *   type:          "scan_start"|"scan_end"
 *   data: {
 *     timestamp:   number
 *   }
 * }
 *
 * @param event Event type (start or end).
 * @param timestamp Datetime in UNIX epoch.
 * @return cJSON object pointer.
 */

cJSON * fim_scan_info_json(fim_scan_event event, long timestamp);

/**
 * @brief Send a scan info event
 *
 * @param event Event type (start or end).
 */
void fim_send_scan_info(fim_scan_event event);

#endif /* SYSCHECK_H */
