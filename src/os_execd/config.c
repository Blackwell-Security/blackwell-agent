/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "execd.h"

char ** wcom_public_keys;

/* Read the config file */
int ExecdConfig(const char *cfgfile)
{
#ifdef WIN32
    int is_disabled = 1;
#else
    int is_disabled = 0;
#endif
    const char *(xmlf[]) = {"ossec_config", "active-response", "disabled", NULL};
    const char *(blocks[]) = {"ossec_config", "active-response", "repeated_offenders", NULL};
    const char *(publickeys[]) = {"ossec_config", "active-response", "public-key", NULL};
    char *disable_entry;
    char *repeated_t;
    char **repeated_a;
    int i;

    OS_XML xml;

    /* Read XML file */
    if (OS_ReadXML(cfgfile, &xml) < 0) {
        merror_exit(XML_ERROR, cfgfile, xml.err, xml.err_line);
    }

    /* We do not validate the xml in here. It is done by other processes. */
    disable_entry = OS_GetOneContentforElement(&xml, xmlf);
    if (disable_entry) {
        if (strcmp(disable_entry, "yes") == 0) {
            is_disabled = 1;
        } else if (strcmp(disable_entry, "no") == 0) {
            is_disabled = 0;
        } else {
            merror(XML_VALUEERR, "disabled", disable_entry);
            return (-1);
        }
    }

    repeated_t = OS_GetOneContentforElement(&xml, blocks);
    if (repeated_t) {
        int i = 0;
        int j = 0;
        repeated_a = OS_StrBreak(',', repeated_t, 5);
        if (!repeated_a) {
            merror(XML_VALUEERR, "repeated_offenders", disable_entry);
            return (-1);
        }

        while (repeated_a[i] != NULL) {
            char *tmpt = repeated_a[i];
            while (*tmpt != '\0') {
                if (*tmpt == ' ' || *tmpt == '\t') {
                    tmpt++;
                } else {
                    break;
                }
            }

            if (*tmpt == '\0') {
                i++;
                continue;
            }

            repeated_offenders_timeout[j] = atoi(tmpt);
            minfo("Adding offenders timeout: %d (for #%d)",
                    repeated_offenders_timeout[j], j + 1);
            j++;
            repeated_offenders_timeout[j] = 0;
            if (j >= 6) {
                break;
            }
            i++;
        }
    }

    if (wcom_public_keys = OS_GetContents(&xml, publickeys), wcom_public_keys) {
        for (i = 0; wcom_public_keys[i]; i++) {
            if (IsFile(wcom_public_keys[i])) {
                merror("Public key file '%s' not found.", wcom_public_keys[i]);
                wcom_public_keys[i][0] = '\0';
            } else {
                mdebug1("Using public key '%s'.", wcom_public_keys[i]);
            }
        }
    } else {
        mdebug1("No public keys defined.");
    }

    OS_ClearXML(&xml);

    return (is_disabled);
}
