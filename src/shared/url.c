/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include <os_net/os_net.h>

int wurl_get(const char * url, const char * dest){
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    char errbuf[CURL_ERROR_SIZE];

    if (curl){
        fp = fopen(dest,"wb");
        if(!fp){
          curl_easy_cleanup(curl);
          return OS_FILERR;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Enable SSL check if url is HTTPS
        if(!strncmp(url,"https",5)){
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
        }

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,errbuf);
        res = curl_easy_perform(curl);

        if(res){
            merror("CURL ERROR %s",errbuf);
            curl_easy_cleanup(curl);
            fclose(fp);
            unlink(dest);
            return OS_CONNERR;
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }

    return 0;
}

int w_download_status(int status,const char *url,const char *dest){

    switch(status){
        case OS_FILERR:
            merror(WURL_WRITE_FILE_ERROR,dest);
            break;
        case OS_CONNERR:
            merror(WURL_DOWNLOAD_FILE_ERROR,url);
            break;
    }

    return status;
}

// Request download
int wurl_request(const char * url, const char * dest) {
    const char * COMMAND = "download";
    char response[64];
    char * _url;
    char * srequest;
    size_t zrequest;
    ssize_t zrecv;
    int sock;
    int retval = -1;

    if (!url) {
        return -1;
    }

    // Escape whitespaces

    _url = wstr_replace(url, " ", "%20");

    // Build request

    zrequest = strlen(_url) + strlen(dest) + strlen(COMMAND) + 3;
    os_malloc(zrequest, srequest);
    snprintf(srequest, zrequest, "%s %s %s", COMMAND, _url, dest);

    // Connect to downlod module

    if (sock = OS_ConnectUnixDomain(isChroot() ? WM_DOWNLOAD_SOCK : WM_DOWNLOAD_SOCK_PATH, SOCK_STREAM, OS_MAXSTR), sock < 0) {
        mwarn("Couldn't connect to download module socket '%s'", WM_DOWNLOAD_SOCK_PATH);
        goto end;
    }

    // Send request

    if (send(sock, srequest, zrequest - 1, 0) != (ssize_t)(zrequest - 1)) {
        merror("Couldn't send request to download module.");
        goto end;
    }

    // Receive response

    switch (zrecv = recv(sock, response, sizeof(response) - 1, 0), zrecv) {
    case -1:
        merror("Couldn't receive URL response from download module.");
        goto end;

    case 0:
        merror("Couldn't receive URL response from download module (closed unexpectedly).");
        goto end;

    default:
        response[zrecv] = '\0';

        // Parse responses

        if (!strcmp(response, "ok")) {
            retval = 0;
        } else if (!strcmp(response, "err connecting to url")) {
            mdebug1(WURL_DOWNLOAD_FILE_ERROR, _url);
            retval = OS_CONNERR;
        } else if (!strcmp(response, "err writing file")) {
            mdebug1(WURL_WRITE_FILE_ERROR, dest);
            retval = OS_FILERR;
        } else {
            mdebug1("Couldn't download from '%s': %s", _url, response);
        }
    }

end:
    free(_url);
    free(srequest);

    if (sock >= 0) {
        close(sock);
    }

    return retval;
}
