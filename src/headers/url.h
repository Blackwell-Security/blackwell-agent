/*
 * URL download support library
 * Copyright (C) 2015-2021, Wazuh Inc.
 * April 3, 2018.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef URL_GET_H_
#define URL_GET_H_

#include <external/curl/include/curl/curl.h>

#define WURL_WRITE_FILE_ERROR "Cannot open file '%s'"
#define WURL_DOWNLOAD_FILE_ERROR "Cannot download file '%s' from URL: '%s'"
#define WURL_TIMEOUT_ERROR  "Timeout reached when downloading file '%s' from URL: '%s'"

typedef struct curl_response {
    char *header;               /* Response header */
    char *body;                 /* Response body */
    long status_code;           /* Response code (200, 404, 500...) */
} curl_response;

int wurl_get(const char * url, const char * dest, const char * header, const char *data, const long timeout);
int w_download_status(int status,const char *url,const char *dest);
// Request download
int wurl_request(const char * url, const char * dest, const char *header, const char *data, const long timeout);
int wurl_request_gz(const char * url, const char * dest, const char * header, const char * data, const long timeout, char *sha256);
char * wurl_http_get(const char * url);
curl_response *wurl_http_request(const char *header, const char *url, const char *payload);
void wurl_free_response(curl_response* response);
#ifndef CLIENT
int wurl_request_bz2(const char * url, const char * dest, const char * header, const char * data, const long timeout, char *sha256);
int wurl_request_uncompress_bz2_gz(const char * url, const char * dest, const char * header, const char * data, const long timeout, char *sha256);
#endif

/* Check download module availability */
int wurl_check_connection();

#endif /* CUSTOM_OUTPUT_SEARCH_H_ */
