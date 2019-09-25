/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include <math.h>
#include <assert.h>
#include "shared.h"
#include "syscheck.h"
#include "syscheck_op.h"
#include "wazuh_modules/wmodules.h"
#include "integrity_op.h"
#include "time_op.h"

// delete this functions
// ==================================
int print_hash_tables();
// ==================================

// Global variables
static int _base_line = 0;

static const char *FIM_EVENT_TYPE[] = {
    "added",
    "deleted",
    "modified"
};

static const char *FIM_EVENT_MODE[] = {
    "scheduled",
    "real-time",
    "whodata"
};


int fim_scan() {
    int position = 0;
    struct timespec start;
    struct timespec end;
    clock_t timeCPU_start = clock();

    gettime(&start);
    minfo(FIM_FREQUENCY_STARTED);
    fim_send_scan_info(FIM_SCAN_START);

    while (syscheck.dir[position] != NULL) {
        minfo("fim_scan(%d): '%s'", FIM_SCHEDULED, syscheck.dir[position]);
        fim_process_event(syscheck.dir[position], FIM_SCHEDULED, NULL);
        position++;
    }

    gettime(&end);

    if (_base_line == 0) {
        _base_line = 1;
    } else {
        check_deleted_files();
    }

    minfo(FIM_FREQUENCY_ENDED);
    fim_send_scan_info(FIM_SCAN_END);

    minfo("The scan has been running during: %.3f sec (%.3f clock sec)",
            time_diff(&start, &end),
            (double)(clock() - timeCPU_start) / CLOCKS_PER_SEC);
    print_hash_tables();

    return 0;
}


int fim_directory (char * path, int dir_position, fim_event_mode mode, whodata_evt * w_evt) {
    DIR *dp;
    struct dirent *entry;
    char *f_name;
    char *s_name;
    char linked_read_file[PATH_MAX + 1] = {'\0'};
    int options;
    size_t path_size;
    short is_nfs;

    if (!path) {
        merror(NULL_ERROR);
        return OS_INVALID;
    }

    options = syscheck.opts[dir_position];

    // Open the directory given
    dp = opendir(path);

    if (!dp) {
        merror(FIM_PATH_NOT_OPEN, path, strerror(errno));
        return (-1);
    }

    // Should we check for NFS?
    if (syscheck.skip_nfs) {
        is_nfs = IsNFS(path);
        if (is_nfs != 0) {
            // Error will be -1, and 1 means skipped
            closedir(dp);
            return (is_nfs);
        }
    }

    if (options & REALTIME_ACTIVE) {
        realtime_adddir(path, 0);
    }

    os_calloc(PATH_MAX + 2, sizeof(char), f_name);
    while ((entry = readdir(dp)) != NULL) {
        *linked_read_file = '\0';

        // Ignore . and ..
        if ((strcmp(entry->d_name, ".") == 0) ||
                (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        strncpy(f_name, path, PATH_MAX);
        path_size = strlen(path);
        s_name = f_name + path_size;

        // Check if the file name is already null terminated
        if (*(s_name - 1) != PATH_SEP) {
            *s_name++ = PATH_SEP;
        }
        *(s_name) = '\0';
        strncpy(s_name, entry->d_name, PATH_MAX - path_size - 2);
#ifdef WIN32
        str_lowercase(f_name);
#endif
        // Process the event related to f_name
        if(fim_process_event(f_name, mode, w_evt) == -1) {
            os_free(f_name);
            closedir(dp);
            return -1;
        }
    }

    os_free(f_name);
    closedir(dp);
    return (0);
}


int fim_check_file (char * file_name, int dir_position, fim_event_mode mode, whodata_evt * w_evt) {
    cJSON * json_event = NULL;
    fim_entry_data * entry_data = NULL;
    fim_entry_data * saved_data = NULL;
    struct stat *file_stat = NULL;
    char * json_formated;
    int options;
    int deleted_flag = 0;

    options = syscheck.opts[dir_position];

    w_mutex_lock(&syscheck.fim_entry_mutex);

    os_calloc(1, sizeof(struct stat), file_stat);
    if (w_stat(file_name, file_stat) < 0) {
        if (options & CHECK_SEECHANGES) {
            delete_target_file(file_name);
        }
        deleted_flag = 1;
    } else {
        //File attributes
        if (entry_data = fim_get_data(file_name, file_stat, mode, options), !entry_data) {
            merror("Couldn't get attributes for file: '%s'", file_name);
            os_free(file_stat);
            return OS_INVALID;
        }
    }

    if (saved_data = (fim_entry_data *) rbtree_get(syscheck.fim_entry, file_name), !saved_data) {
        if (!deleted_flag) {
            // New entry. Insert into hash table
            if (fim_insert(file_name, entry_data, file_stat) == -1) {
                free_entry_data(entry_data);
                os_free(file_stat);
                w_mutex_unlock(&syscheck.fim_entry_mutex);
                return OS_INVALID;
            }

            if (_base_line) {
                json_event = fim_json_event(file_name, NULL, entry_data, dir_position, FIM_ADD, mode, w_evt);
            }
        }
    } else {
        // Delete file. Sending alert.
        if (deleted_flag) {
            json_event = fim_json_event(file_name, NULL, saved_data, dir_position, FIM_DELETE, mode, w_evt);
            fim_delete(file_name);
        // Checking for changes
        } else {
            saved_data->scanned = 1;
            if (json_event = fim_json_event(file_name, saved_data, entry_data, dir_position, FIM_MODIFICATION, mode, w_evt), json_event) {
                if (fim_update(file_name, entry_data) == -1) {
                    free_entry_data(entry_data);
                    os_free(file_stat);
                    w_mutex_unlock(&syscheck.fim_entry_mutex);
                    return OS_INVALID;
                }
            } else {
                free_entry_data(entry_data);
            }
        }
    }
    w_mutex_unlock(&syscheck.fim_entry_mutex);

    if (!_base_line && options & CHECK_SEECHANGES && !deleted_flag) {
        // The first backup is created
        seechanges_addfile(file_name);
    }

    if (json_event && _base_line) {
        json_formated = cJSON_PrintUnformatted(json_event);
        send_syscheck_msg(json_formated);
        os_free(json_formated);
        cJSON_Delete(json_event);
    }
    os_free(file_stat);

    return 0;
}


int fim_process_event(char * file, fim_event_mode mode, whodata_evt *w_evt) {
    struct stat file_stat;
    int dir_position = 0;
    int depth = 0;

    mdebug1("~~ Process event(mode:%d): '%s'", mode, file);

    if (fim_check_ignore(file) == 1) {
        return (0);
    }

    if (fim_check_restrict (file, syscheck.filerestrict[dir_position]) == 1) {
        return (0);
    }

    // If the directory have another configuration will come back
    if (dir_position = fim_configuration_directory(file), dir_position < 0) {
        minfo("~~ No configuration founded for file: '%s'", file);
        return(0);
    }

    // We need to process every event generated by scheduled scans because we need to alert about discarded events of real-time and Whodata mode
    if (mode == FIM_SCHEDULED || mode == FIM_MODE(syscheck.opts[dir_position])) {
        depth = fim_check_depth(file, dir_position);
        //minfo("~~Depth from parent path: '%d' recursion level:'%d'", depth, syscheck.recursion_level[dir_position]);
        if(depth >= syscheck.recursion_level[dir_position]) {
            minfo("~~ Maximum depth reached: %s", file);
            return 0;
        }

        // If w_stat fails can be a deleted file
        if (w_stat(file, &file_stat) < 0) {
            // Regular file
            if (fim_check_file(file, dir_position, mode, w_evt) < 0) {
                merror("Skiping file: '%s'", file);
            }
        } else {
            switch(file_stat.st_mode & S_IFMT) {
                case FIM_REGULAR:
                    // Regular file
                    if (fim_check_file(file, dir_position, mode, w_evt) < 0) {
                        merror("Skip event: '%s'", file);
                    }
                    break;

                case FIM_DIRECTORY:
                    // Directory path
                    fim_directory(file, dir_position, mode, w_evt);
                    break;
#ifndef WIN32
                case FIM_LINK:
                    // Symbolic links add link and follow if it is configured
                    // TODO: implement symbolic links
                    break;
#endif
                default:
                    mdebug1("~~ Unsupported file type(mode:%d): '%s'", mode, file);
                    // Unsupported file type
                    return -1;
            }
        }
    } else {
        minfo("~~~~ Different configuration applied to file '%s'", file);
    }

    return 0;
}


void fim_audit_inode_event(whodata_evt * w_evt) {
    fim_inode_data * inode_data;
    struct stat file_stat;
    char *key_inodehash;

    mdebug1("~~ Inode event: (%s)'%s'", w_evt->inode, w_evt->path);

    os_calloc(OS_SIZE_128, sizeof(char), key_inodehash);
    snprintf(key_inodehash, OS_SIZE_128, "%s:%s", w_evt->dev, w_evt->inode);

    w_mutex_lock(&syscheck.fim_entry_mutex);

    if (inode_data = OSHash_Get_ex(syscheck.fim_inode, key_inodehash), inode_data) {
        char **event_paths = NULL;
        int i = 0;

        os_calloc(inode_data->items, sizeof(char*), event_paths);

        while(inode_data->paths && inode_data->paths[i] && i < inode_data->items) {
            os_strdup(inode_data->paths[i], event_paths[i]);
            i++;
        }

        w_mutex_unlock(&syscheck.fim_entry_mutex);

        if (w_evt->path) {
            fim_process_event(w_evt->path, FIM_WHODATA, w_evt);
        }

        for(; i > 0; i--) {
            fim_process_event(event_paths[i - 1], FIM_WHODATA, w_evt);
            os_free(event_paths[i - 1]);
        }
        os_free(event_paths);
    } else {
        w_mutex_unlock(&syscheck.fim_entry_mutex);

        if (w_stat(w_evt->path, &file_stat) < 0) {
            mdebug1("~~~~ File '%s' not in database and stat failed", w_evt->path);
        } else {
            switch(file_stat.st_mode & S_IFMT) {
                case FIM_REGULAR:
                    // Regular file
                    fim_process_event(w_evt->path, FIM_WHODATA, w_evt);
                    break;

                case FIM_DIRECTORY:
                    // Directory path
                    minfo("~~~~ Event path is a directory, discarding '%s'", w_evt->path);
                    break;
                // TODO: Case for symbolic links?
                default:
                    // Unsupported file type
                    break;
            }
        }
    }

    os_free(key_inodehash);
    return;
}


// Returns the position of the path into directories array
int fim_configuration_directory(char * path) {
    int it = 0;
    int max = 0;
    int res = 0;
    int position = -1;

    while(syscheck.dir[it]) {
        res = w_compare_str(syscheck.dir[it], path);
        if (max < res) {
            position = it;
            max = res;
        }
        it++;
    }

    return position;
}


// Evaluates the depth of the directory or file to check if it exceeds the configured max_depth value
int fim_check_depth(char * path, int dir_position) {
    char * pos;
    int depth = 0;
    unsigned int parent_path_size;

    if (!syscheck.dir[dir_position]) {
        minfo("~~Invalid parent path.");
        return -1;
    }

    parent_path_size = strlen(syscheck.dir[dir_position]);

    if (parent_path_size > strlen(path)) {
        minfo("~~Parent directory < path: %s < %s", syscheck.dir[dir_position], path);
        return -1;
    }

    pos = path + parent_path_size;
    while (pos) {
        if (pos = strchr(pos, '/'), pos) {
            depth++;
        } else {
            break;
        }
        pos++;
    }

    return depth;
}


// Get data from file
fim_entry_data * fim_get_data (const char * file_name, struct stat *file_stat, fim_event_mode mode, int options) {
    fim_entry_data * data = NULL;

    os_calloc(1, sizeof(fim_entry_data), data);
    init_fim_data_entry(data);

    if (options & CHECK_SIZE) {
        data->size = file_stat->st_size;
    }

    if (options & CHECK_PERM) {
#ifdef WIN32
        int error;
        char perm_unescaped[OS_SIZE_6144 + 1];
        if (error = w_get_file_permissions(file_name, perm_unescaped, OS_SIZE_6144), error) {
            merror(FIM_ERROR_EXTRACT_PERM, file_name, error);
        } else {
            data->perm = escape_perm_sum(perm_unescaped);
        }
#else
        data->perm = agent_file_perm(file_stat->st_mode);
#endif
    }

    if (options & CHECK_MTIME) {
        data->mtime = file_stat->st_mtime;
    }

    if (options & CHECK_INODE) {
        data->inode = file_stat->st_ino;
    }

#ifdef WIN32
    if (options & CHECK_OWNER) {
        data->user_name = get_user(file_name, 0, &data->uid);
    }
#else
    if (options & CHECK_OWNER) {
        char aux[OS_SIZE_64];
        snprintf(aux, OS_SIZE_64, "%u", file_stat->st_uid);
        os_strdup(aux, data->uid);

        data->user_name = get_user(file_name, file_stat->st_uid, NULL);
    }

    if (options & CHECK_GROUP) {
        char aux[OS_SIZE_64];
        snprintf(aux, OS_SIZE_64, "%u", file_stat->st_gid);
        os_strdup(aux, data->gid);

        os_strdup((char*)get_group(file_stat->st_gid), data->group_name);
    }
#endif

    snprintf(data->hash_md5, sizeof(os_md5), "%s", "d41d8cd98f00b204e9800998ecf8427e");
    snprintf(data->hash_sha1, sizeof(os_sha1), "%s", "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    snprintf(data->hash_sha256, sizeof(os_sha256), "%s", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // The file exists and we don't have to delete it from the hash tables
    data->scanned = 1;

    // We won't calculate hash for symbolic links, empty or large files
#ifdef __linux__
    if ((file_stat->st_mode & S_IFMT) == FIM_REGULAR)
#endif
        if (file_stat->st_size > 0 &&
                (size_t)file_stat->st_size < syscheck.file_max_size &&
                (options & CHECK_MD5SUM || options & CHECK_SHA1SUM || options & CHECK_SHA256SUM) ) {
            if (OS_MD5_SHA1_SHA256_File(file_name,
                                        syscheck.prefilter_cmd,
                                        data->hash_md5,
                                        data->hash_sha1,
                                        data->hash_sha256,
                                        OS_BINARY,
                                        syscheck.file_max_size) < 0) {
                merror("Couldn't generate hashes for '%s'", file_name);
                free_entry_data(data);
                return NULL;
        }
    }

    if (!(options & CHECK_MD5SUM)) {
        data->hash_md5[0] = '\0';
    }

    if (!(options & CHECK_SHA1SUM)) {
        data->hash_sha1[0] = '\0';
    }

    if (!(options & CHECK_SHA256SUM)) {
        data->hash_sha256[0] = '\0';
    }

    data->dev = file_stat->st_dev;
    data->mode = mode;
    data->options = options;
    data->last_event = time(NULL);
    data->scanned = 1;
    fim_get_checksum(data);

    return data;
}


// Initialize fim_entry_data structure
void init_fim_data_entry(fim_entry_data *data) {
    data->size = 0;
    data->perm = NULL;
    data->uid = NULL;
    data->gid = NULL;
    data->user_name = NULL;
    data->group_name = NULL;
    data->mtime = 0;
    data->inode = 0;
    data->hash_md5[0] = '\0';
    data->hash_sha1[0] = '\0';
    data->hash_sha256[0] = '\0';
}


// Returns checksum string
void fim_get_checksum (fim_entry_data * data) {
    char *checksum = NULL;
    int size;

    os_calloc(OS_SIZE_128, sizeof(char), checksum);

    size = snprintf(checksum,
            OS_SIZE_128,
            "%d:%s:%s:%s:%s:%s:%d:%lu:%s:%s:%s",
            data->size,
            data->perm,
            data->uid,
            data->gid,
            data->user_name,
            data->group_name,
            data->mtime,
            data->inode,
            data->hash_md5,
            data->hash_sha1,
            data->hash_sha256);

    if (size < 0) {
        merror("Wrong size, can't get checksum: %s", checksum);
        *checksum = '\0';
    } else if (size >= OS_SIZE_128) {
        // Needs more space
        os_realloc(checksum, (size + 1) * sizeof(char), checksum);
        snprintf(checksum,
                size + 1,
                "%d:%s:%s:%s:%s:%s:%d:%lu:%s:%s:%s",
                data->size,
                data->perm,
                data->uid,
                data->gid,
                data->user_name,
                data->group_name,
                data->mtime,
                data->inode,
                data->hash_md5,
                data->hash_sha1,
                data->hash_sha256);
    }

    OS_SHA1_Str(checksum, sizeof(checksum), data->checksum);
    free(checksum);
}


// Inserts a file in the syscheck hash table structure (inodes and paths)
int fim_insert (char * file, fim_entry_data * data, struct stat *file_stat) {
    char * inode_key = NULL;

    if (rbtree_insert(syscheck.fim_entry, file, data) == NULL) {
        minfo("Duplicate path: '%s'", file);
        return (-1);
    }

#ifndef WIN32
    fim_inode_data * inode_data;

    // Function OSHash_Add_ex doesn't alloc memory for the data of the hash table
    os_calloc(OS_SIZE_128, sizeof(char), inode_key);
    snprintf(inode_key, OS_SIZE_128, "%ld:%ld", file_stat->st_dev, file_stat->st_ino);

    if (inode_data = OSHash_Get(syscheck.fim_inode, inode_key), !inode_data) {
        os_calloc(1, sizeof(fim_inode_data), inode_data);

        inode_data->paths = os_AddStrArray(file, inode_data->paths);
        inode_data->items = 1;

        if (OSHash_Add(syscheck.fim_inode, inode_key, inode_data) != 2) {
            merror("Unable to add inode to db: '%s' => '%s'", inode_key, file);
            os_free(inode_key);
            return (-1);
        }
    } else {
        if (!os_IsStrOnArray(file, inode_data->paths)) {
            inode_data->paths = os_AddStrArray(file, inode_data->paths);
            inode_data->items++;
        }
    }
#endif

    os_free(inode_key);
    return 0;
}


// Update an entry in the syscheck hash table structure (inodes and paths)
int fim_update (char * file, fim_entry_data * data) {
    char * inode_key;

    os_calloc(OS_SIZE_128, sizeof(char), inode_key);
    snprintf(inode_key, OS_SIZE_128, "%ld:%ld", data->dev, data->inode);

    if (!file || strcmp(file, "") == 0 || !inode_key || strcmp(inode_key, "") == 0) {
        merror("Can't update entry invalid file or inode");
        // TODO: Consider if we should exit here. Change to debug message
    }

    if (rbtree_replace(syscheck.fim_entry, file, data) == NULL) {
        merror("Unable to update file to db, key not found: '%s'", file);
        os_free(inode_key);
        return (-1);
    }
    os_free(inode_key);
    return 0;
}


// Deletes a path from the syscheck hash table structure and sends a deletion event
int fim_delete (char * file_name) {
    fim_entry_data * data;

    if (data = rbtree_get(syscheck.fim_entry, file_name), data) {
#ifndef WIN32
        char * inode_key = NULL;
        os_calloc(OS_SIZE_128, sizeof(char), inode_key);
        snprintf(inode_key, OS_SIZE_128, "%ld:%ld", data->dev, data->inode);
        delete_inode_item(inode_key, file_name);
        os_free(inode_key);
#endif
        rbtree_delete(syscheck.fim_entry, file_name);
    }

    return 0;
}


// Deletes a path from the syscheck hash table structure and sends a deletion event on scheduled scans
void check_deleted_files() {
    cJSON * json_event = NULL;
    char * json_formated;
    char ** keys;
    int i;
    int pos;

    w_mutex_lock(&syscheck.fim_entry_mutex);
    keys = rbtree_keys(syscheck.fim_entry);
    w_mutex_unlock(&syscheck.fim_entry_mutex);

    for (i = 0; keys[i] != NULL; i++) {

        w_mutex_lock(&syscheck.fim_entry_mutex);

        fim_entry_data * data = rbtree_get(syscheck.fim_entry, keys[i]);

        if (!data) {
            w_mutex_unlock(&syscheck.fim_entry_mutex);
            continue;
        }

        // File doesn't exist so we have to delete it from the
        // hash tables and send a deletion event.
        if (!data->scanned) {
            minfo("File '%s' has been deleted.", keys[i]);

            if (pos = fim_configuration_directory(keys[i]), pos < 0) {
                w_mutex_unlock(&syscheck.fim_entry_mutex);
                continue;
            }

            json_event = fim_json_event (keys[i], NULL, data, pos, FIM_DELETE, FIM_SCHEDULED, NULL);
            fim_delete(keys[i]);

            if (json_event && _base_line) {
                json_formated = cJSON_PrintUnformatted(json_event);
                send_syscheck_msg(json_formated);
                os_free(json_formated);
                cJSON_Delete(json_event);
            }
        } else {
             // File still exists. We only need to reset the scanned flag.
            data->scanned = 0;
        }

        w_mutex_unlock(&syscheck.fim_entry_mutex);

    }

    free_strarray(keys);

    return;
}


void delete_inode_item(char *inode_key, char *file_name) {
    fim_inode_data *inode_data;
    char **new_paths;
    int i = 0;

    if (inode_data = OSHash_Get(syscheck.fim_inode, inode_key), inode_data) {
        // If it's the last path we can delete safely the hash node
        if(inode_data->items == 1) {
            if(inode_data = OSHash_Delete(syscheck.fim_inode, inode_key), inode_data) {
                free_inode_data(inode_data);
            }
        }
        // We must delete only file_name from paths
        else {
            os_calloc(inode_data->items - 1, sizeof(char*), new_paths);
            for(i = 0; i < inode_data->items; i++) {
                if(strcmp(inode_data->paths[i], file_name)) {
                    new_paths = os_AddStrArray(inode_data->paths[i], new_paths);
                }
            }

            free_strarray(inode_data->paths);
            inode_data->paths = new_paths;
            inode_data->items--;
        }
    }
}

cJSON * fim_json_event(char * file_name, fim_entry_data * old_data, fim_entry_data * new_data, int dir_position, fim_event_type type, fim_event_mode mode, whodata_evt * w_evt) {
    cJSON * changed_attributes = NULL;

    if (old_data != NULL) {
        changed_attributes = fim_json_compare_attrs(old_data, new_data);

        // If no such changes, do not send event.

        if (cJSON_GetArraySize(changed_attributes) == 0) {
            cJSON_Delete(changed_attributes);
            return NULL;
        }
    }

    cJSON * json_event = cJSON_CreateObject();
    cJSON_AddStringToObject(json_event, "type", "event");

    cJSON * data = cJSON_CreateObject();
    cJSON_AddItemToObject(json_event, "data", data);

    cJSON_AddStringToObject(data, "path", file_name);
    cJSON_AddStringToObject(data, "mode", FIM_EVENT_MODE[mode]);
    cJSON_AddStringToObject(data, "type", FIM_EVENT_TYPE[type]);
    cJSON_AddNumberToObject(data, "timestamp", new_data->last_event);

    char * tags = syscheck.tag[dir_position];

    if (tags != NULL) {
        cJSON_AddStringToObject(data, "tags", tags);
    }

    if (syscheck.opts[dir_position] & CHECK_SEECHANGES && type != 1) {
        char * diff = seechanges_addfile(file_name);

        if (diff != NULL) {
            cJSON_AddStringToObject(data, "content_changes", diff);
            os_free(diff);
        }
    }

    if (old_data) {
        cJSON_AddItemToObject(data, "changed_attributes", changed_attributes);
        cJSON_AddItemToObject(data, "old_attributes", fim_attributes_json(old_data));
    }

    cJSON_AddItemToObject(data, "attributes", fim_attributes_json(new_data));

    if (w_evt) {
        cJSON_AddItemToObject(data, "audit", fim_audit_json(w_evt));
    }

    return json_event;
}

// Create file attribute set JSON from a FIM entry structure

cJSON * fim_attributes_json(const fim_entry_data * data) {
    cJSON * attributes = cJSON_CreateObject();

    // TODO: Read structure.
    cJSON_AddStringToObject(attributes, "type", "file");

    if (data->options & CHECK_SIZE) {
        cJSON_AddNumberToObject(attributes, "size", data->size);
    }

    if (data->options & CHECK_PERM) {
        cJSON_AddStringToObject(attributes, "perm", data->perm);
    }

    if (data->options & CHECK_OWNER) {
        cJSON_AddStringToObject(attributes, "uid", data->uid);
    }

    if (data->options & CHECK_GROUP) {
        cJSON_AddStringToObject(attributes, "gid", data->gid);
    }

    if (data->user_name) {
        cJSON_AddStringToObject(attributes, "user_name", data->user_name);
    }

    if (data->group_name) {
        cJSON_AddStringToObject(attributes, "group_name", data->group_name);
    }

    if (data->options & CHECK_INODE) {
        cJSON_AddNumberToObject(attributes, "inode", data->inode);
    }

    if (data->options & CHECK_MTIME) {
        cJSON_AddNumberToObject(attributes, "mtime", data->mtime);
    }

    if (data->options & CHECK_MD5SUM) {
        cJSON_AddStringToObject(attributes, "hash_md5", data->hash_md5);
    }

    if (data->options & CHECK_SHA1SUM) {
        cJSON_AddStringToObject(attributes, "hash_sha1", data->hash_sha1);
    }

    if (data->options & CHECK_SHA256SUM) {
        cJSON_AddStringToObject(attributes, "hash_sha256", data->hash_sha256);
    }

    // TODO: Read structure.
    if (data->options & CHECK_ATTRS) {
        cJSON_AddStringToObject(attributes, "win_attributes", "");
    }

    if (*data->checksum) {
        cJSON_AddStringToObject(attributes, "checksum", data->checksum);
    }

    return attributes;
}

// Create file entry JSON from a FIM entry structure

cJSON * fim_entry_json(const char * path, fim_entry_data * data) {
    assert(data);
    assert(path);

    cJSON * root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "path", path);
    cJSON_AddNumberToObject(root, "timestamp", data->last_event);

    cJSON * attributes = fim_attributes_json(data);
    cJSON_AddItemToObject(root, "attributes", attributes);

    return root;
}

// Create file attribute comparison JSON object

cJSON * fim_json_compare_attrs(const fim_entry_data * old_data, const fim_entry_data * new_data) {
    cJSON * changed_attributes = cJSON_CreateArray();

    if ( (old_data->options & CHECK_SIZE) && (old_data->size != new_data->size) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("size"));
    }

    if ( (old_data->options & CHECK_PERM) && strcmp(old_data->perm, new_data->perm) != 0 ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("permission"));
    }

    if (old_data->options & CHECK_OWNER) {
        if (old_data->uid && new_data->uid && strcmp(old_data->uid, new_data->uid) != 0) {
            cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("uid"));
        }

        if (old_data->user_name && new_data->user_name && strcmp(old_data->user_name, new_data->user_name) != 0) {
            cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("user_name"));
        }
    }

    if (old_data->options & CHECK_GROUP) {
        if (old_data->gid && new_data->gid && strcmp(old_data->gid, new_data->gid) != 0) {
            cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("gid"));
        }

        if (old_data->group_name && new_data->group_name && strcmp(old_data->group_name, new_data->group_name) != 0) {
            cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("group_name"));
        }
    }

    if ( (old_data->options & CHECK_MTIME) && (old_data->mtime != new_data->mtime) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("mtime"));
    }

#ifdef __linux__
    if ( (old_data->options & CHECK_INODE) && (old_data->inode != new_data->inode) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("inode"));
    }
#endif

    if ( (old_data->options & CHECK_MD5SUM) && (strcmp(old_data->hash_md5, new_data->hash_md5) != 0) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("md5"));
    }

    if ( (old_data->options & CHECK_SHA1SUM) && (strcmp(old_data->hash_sha1, new_data->hash_sha1) != 0) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("sha1"));
    }

    if ( (old_data->options & CHECK_SHA256SUM) && (strcmp(old_data->hash_sha256, new_data->hash_sha256) != 0) ) {
        cJSON_AddItemToArray(changed_attributes, cJSON_CreateString("sha256"));
    }

    return changed_attributes;
}

// Create file audit data JSON object

cJSON * fim_audit_json(const whodata_evt * w_evt) {
    cJSON * fim_audit = cJSON_CreateObject();

    cJSON_AddStringToObject(fim_audit, "user_id", w_evt->user_id);
    cJSON_AddStringToObject(fim_audit, "user_name", w_evt->user_name);
    cJSON_AddStringToObject(fim_audit, "group_id", w_evt->group_id);
    cJSON_AddStringToObject(fim_audit, "group_name", w_evt->group_name);
    cJSON_AddStringToObject(fim_audit, "process_name", w_evt->process_name);
    cJSON_AddStringToObject(fim_audit, "path", w_evt->path);
    cJSON_AddStringToObject(fim_audit, "audit_uid", w_evt->audit_uid);
    cJSON_AddStringToObject(fim_audit, "audit_name", w_evt->audit_name);
    cJSON_AddStringToObject(fim_audit, "effective_uid", w_evt->effective_uid);
    cJSON_AddStringToObject(fim_audit, "effective_name", w_evt->effective_name);
    cJSON_AddNumberToObject(fim_audit, "ppid", w_evt->ppid);
    cJSON_AddNumberToObject(fim_audit, "process_id", w_evt->process_id);

    return fim_audit;
}

int fim_check_ignore (const char *file_name) {
    /* Check if the file should be ignored */
    if (syscheck.ignore) {
        int i = 0;
        while (syscheck.ignore[i] != NULL) {
            if (strncasecmp(syscheck.ignore[i], file_name, strlen(syscheck.ignore[i])) == 0) {
                mdebug2(FIM_IGNORE_ENTRY, "file", file_name, syscheck.ignore[i]);
                return (1);
            }
            i++;
        }
    }

    /* Check in the regex entry */
    if (syscheck.ignore_regex) {
        int i = 0;
        while (syscheck.ignore_regex[i] != NULL) {
            if (OSMatch_Execute(file_name, strlen(file_name), syscheck.ignore_regex[i])) {
                mdebug2(FIM_IGNORE_SREGEX, "file", file_name, syscheck.ignore_regex[i]->raw);
                return (1);
            }
            i++;
        }
    }

    return (0);
}

int fim_check_restrict (const char *file_name, OSMatch *restriction) {
    /* Restrict file types */
    if (restriction) {
        if (!OSMatch_Execute(file_name, strlen(file_name), restriction)) {
            mdebug1(FIM_FILE_IGNORE_RESTRICT, file_name);
            return (1);
        }
    }

    return (0);
}


void free_entry_data(fim_entry_data * data) {
    if (!data) {
        return;
    }
    if (data->perm) {
        os_free(data->perm);
    }
    if (data->uid) {
#ifdef WIN32
        LocalFree(data->uid);
#else
        os_free(data->uid);
#endif
    }
    if (data->gid) {
        os_free(data->gid);
    }
    if (data->user_name) {
        os_free(data->user_name);
    }
    if (data->group_name) {
        os_free(data->group_name);
    }
    os_free(data);
}


void free_inode_data(fim_inode_data * data) {
    int i;

    if (!data) {
        return;
    }

    for (i = 0; i < data->items; i++) {
        os_free(data->paths[i]);
    }
    os_free(data->paths);
    os_free(data);
}


/* ================================================================================================ */
/* ================================================================================================ */
/* ================================================================================================ */
/* ================================================================================================ */


int print_hash_tables() {
    int element_sch = 0;
    int element_rt = 0;
    int element_wd = 0;
    int element_total = 0;
    char ** keys = rbtree_keys(syscheck.fim_entry);
    int i;

    for (i = 0; keys[i]; i++) {
        fim_entry_data * data = rbtree_get(syscheck.fim_entry, keys[i]);
        assert(data != NULL);

        //minfo("ENTRY (%d) => '%s'->'%lu' scanned:'%u'\n", element_total, keys[i], data->inode, data->scanned);

        switch(data->mode) {
            case FIM_SCHEDULED: element_sch++; break;
            case FIM_REALTIME: element_rt++; break;
            case FIM_WHODATA: element_wd++; break;
        }

        element_total++;
    }

#ifndef WIN32
    OSHashNode * hash_node;
    unsigned int inode_it = 0;
    unsigned inode_items = 0;
    unsigned element_totali = 0;
    char * files = NULL;

    for (hash_node = OSHash_Begin(syscheck.fim_inode, &inode_it); hash_node; hash_node = OSHash_Next(syscheck.fim_inode, &inode_it, hash_node)) {
        fim_inode_data * data = hash_node->data;
        os_free(files);
        os_calloc(1, sizeof(char), files);
        *files = '\0';

        for (i = 0; i < data->items; i++) {
            wm_strcat(&files, data->paths[i], ',');
        }

        mdebug1("INODE (%u) => '%s'->(%d)'%s'\n", element_totali, (char*)hash_node->key, data->items, files);
        element_totali++;
        inode_items += data->items;
    }
    minfo("SCH '%d'", element_sch);
    minfo("RT '%d'", element_rt);
    minfo("WD '%d'", element_wd);
    minfo("Total: '%d' inodes: '%d' element_totali:'%d'", element_total, inode_items, element_totali);

    os_free(files);
#endif
    free_strarray(keys);

    return 0;
}

// Create scan info JSON event

cJSON * fim_scan_info_json(fim_scan_event event, long timestamp) {
    cJSON * root = cJSON_CreateObject();
    cJSON * data = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", event == FIM_SCAN_START ? "scan_start" : "scan_end");
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddNumberToObject(data, "timestamp", timestamp);

    return root;
}

// Send a scan info event

void fim_send_scan_info(fim_scan_event event) {
    cJSON * json = fim_scan_info_json(event, time(NULL));
    char * plain = cJSON_PrintUnformatted(json);

    send_syscheck_msg(plain);

    free(plain);
    cJSON_Delete(json);
}
