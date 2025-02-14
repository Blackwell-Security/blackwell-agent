# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import copy
import json
import os
import re
import socket
import ssl
from collections import OrderedDict
from datetime import datetime
from enum import Enum
from os.path import exists
from typing import Dict, Optional, Union

import certifi
import httpx
import blackwell
from api import configuration
from blackwell import BlackwellError, BlackwellException, BlackwellInternalError
from blackwell.core import common
from blackwell.core.cluster.utils import get_manager_status
from blackwell.core.configuration import get_active_configuration, get_cti_url
from blackwell.core.utils import get_utc_now, get_utc_strptime, tail
from blackwell.core.blackwell_socket import BlackwellSocket


_re_logtest = re.compile(r"^.*(?:ERROR: |CRITICAL: )(?:\[.*\] )?(.*)$")

OSSEC_LOG_FIELDS = ['timestamp', 'tag', 'level', 'description']
CTI_URL = get_cti_url()
RELEASE_UPDATES_URL = os.path.join(CTI_URL, 'api', 'v1', 'ping')
ONE_DAY_SLEEP = 60 * 60 * 24
BLACKWELL_UID_KEY = 'blackwell-uid'
BLACKWELL_TAG_KEY = 'blackwell-tag'
USER_AGENT_KEY = 'user-agent'

class LoggingFormat(Enum):
    plain = "plain"
    json = "json"


def status() -> dict:
    """Return the Manager processes that are running."""

    return get_manager_status()


def get_ossec_log_fields(log: str, log_format: LoggingFormat = LoggingFormat.plain) -> Union[tuple, None]:
    """Get ossec.log log fields.

    Parameters
    ----------
    log : str
        Log example.
    log_format : LoggingFormat
        Blackwell log format.

    Returns
    -------
    tuple or None
        Log fields: timestamp, tag, level, and description.
    """
    if log_format == LoggingFormat.plain:
        regex_category = re.compile(
            r"^(\d\d\d\d/\d\d/\d\d\s\d\d:\d\d:\d\d)\s(\S+)(?:\[.*)?:\s(DEBUG|INFO|CRITICAL|ERROR|WARNING):(.*)$")

        match = re.search(regex_category, log)
        if not match:
            return None

        date = match.group(1)
        tag = match.group(2)
        level = match.group(3)
        description = match.group(4)

    elif log_format == LoggingFormat.json:
        try:
            match = json.loads(log)
        except json.decoder.JSONDecodeError:
            return None

        try:
            date = match['timestamp']
            tag = match['tag']
            level = match['level']
            description = match['description']
        except KeyError:
            return None
    else:
        return None

    if "rootcheck" in tag:  # Unify rootcheck category
        tag = "blackwell-rootcheck"

    return get_utc_strptime(date, '%Y/%m/%d %H:%M:%S'), tag, level.lower(), description


def get_blackwell_active_logging_format() -> LoggingFormat:
    """Obtain the Blackwell active logging format.

    Returns
    -------
    LoggingFormat
        Blackwell active log format. Can either be `plain` or `json`. If it has both types, `plain` will be returned.
    """
    active_logging = get_active_configuration(agent_id="000", component="com", configuration="logging")['logging']
    return LoggingFormat.plain if active_logging['plain'] == "yes" else LoggingFormat.json


def get_ossec_logs(limit: int = 2000) -> list:
    """Return last <limit> lines of ossec.log file.

    Parameters
    ----------
    limit : int
        Number of lines to return. Default: 2000

    Returns
    -------
    list
        List of dictionaries with requested logs.
    """
    logs = []

    log_format = get_blackwell_active_logging_format()
    if log_format == LoggingFormat.plain and exists(common.BLACKWELL_LOG):
        blackwell_log_content = tail(common.BLACKWELL_LOG, limit)
    elif log_format == LoggingFormat.json and exists(common.BLACKWELL_LOG_JSON):
        blackwell_log_content = tail(common.BLACKWELL_LOG_JSON, limit)
    else:
        raise BlackwellInternalError(1020)

    for line in blackwell_log_content:
        log_fields = get_ossec_log_fields(line, log_format=log_format)
        if log_fields:
            date, tag, level, description = log_fields

            # We transform local time (ossec.log) to UTC with ISO8601 maintaining time integrity
            log_line = {'timestamp': date.strftime(common.DATE_FORMAT), 'tag': tag,
                        'level': level, 'description': description}
            logs.append(log_line)

    return logs


def get_logs_summary(limit: int = 2000) -> dict:
    """Get the number of alerts of each tag.

    Parameters
    ----------
    limit : int
        Number of lines to return. Default: 2000

    Returns
    -------
    dict
        Number of logs for every tag.
    """
    tags = dict()
    logs = get_ossec_logs(limit)

    for log in logs:
        if log['tag'] in tags:
            tags[log['tag']]['all'] += 1
        else:
            tags[log['tag']] = {'all': 1, 'info': 0, 'error': 0, 'critical': 0, 'warning': 0, 'debug': 0}
        tags[log['tag']][log['level']] += 1

    return tags


def validate_ossec_conf() -> str:
    """Check if Blackwell configuration is OK.

    Raises
    ------
    BlackwellInternalError(1014)
        If there is a socket communication error.
    BlackwellInternalError(1013)
        If it is unable to connect to socket.
    BlackwellInternalError(1901)
        If 'execq' socket cannot be created.
    BlackwellInternalError(1904)
        If there is bad data received from 'execq'.

    Returns
    -------
    str
        Status of the configuration.
    """

    # Socket path
    wcom_socket_path = common.WCOM_SOCKET
    # Message for checking Blackwell configuration
    wcom_msg = common.CHECK_CONFIG_COMMAND

    # Connect to wcom socket
    if exists(wcom_socket_path):
        try:
            wcom_socket = BlackwellSocket(wcom_socket_path)
        except BlackwellException as e:
            extra_msg = f'Socket: BLACKWELL_PATH/queue/sockets/com. Error {e.message}'
            raise BlackwellInternalError(1013, extra_message=extra_msg)
    else:
        raise BlackwellInternalError(1901)

    # Send msg to wcom socket
    try:
        wcom_socket.send(wcom_msg.encode())

        buffer = bytearray()
        datagram = wcom_socket.receive()
        buffer.extend(datagram)

    except (socket.error, socket.timeout) as e:
        raise BlackwellInternalError(1014, extra_message=str(e))
    finally:
        wcom_socket.close()

    try:
        response = parse_execd_output(buffer.decode('utf-8').rstrip('\0'))
    except (KeyError, json.decoder.JSONDecodeError) as e:
        raise BlackwellInternalError(1904, extra_message=str(e))

    return response


def parse_execd_output(output: str) -> Dict:
    """Parse output from execd socket to fetch log message and remove log date, log daemon, log level, etc.

    Parameters
    ----------
    output : str
        Raw output from execd.

    Returns
    -------
    dict
        Cleaned log message in a dictionary structure.
    """
    json_output = json.loads(output)
    error_flag = json_output['error']
    if error_flag != 0:
        errors = []
        log_lines = json_output['message'].splitlines(keepends=False)
        for line in log_lines:
            match = _re_logtest.match(line)
            if match:
                errors.append(match.group(1))
        errors = list(OrderedDict.fromkeys(errors))
        raise BlackwellError(1908, extra_message=', '.join(errors))
    else:
        response = {'status': 'OK'}

    return response


def get_api_conf() -> dict:
    """Return current API configuration.

    Returns
    -------
    dict
        API configuration.
    """
    return copy.deepcopy(configuration.api_conf)


def _get_ssl_context() -> ssl.SSLContext:
    """Return a default ssl context."""
    return ssl.create_default_context(cafile=certifi.where())


def get_update_information_template(
        uuid: str,
        update_check: bool,
        current_version: str = f"v{blackwell.__version__}",
        last_check_date: Optional[datetime] = None
) -> dict:
    """Build and return a template for the update_information dict.

    Parameters
    ----------
    uuid : str
        Blackwell UID to include in the result.
    update_check : bool
        Indicates if the check is enabled or not.
    current_version : str, optional
        Indicates the current version of Blackwell, by default blackwell.__version__.
    last_check_date : Optional[datetime], optional
        Indicates the datetime of the last check, by default None.

    Returns
    -------
    dict
        Template with the given data.
    """
    return {
        'uuid': uuid,
        'last_check_date': last_check_date if last_check_date is not None else '',
        'current_version': current_version,
        'update_check': update_check,
        'last_available_major': {},
        'last_available_minor': {},
        'last_available_patch': {},
    }


async def query_update_check_service(installation_uid: str) -> dict:
    """Make a query to the update check service and retrieve updates information.

    Parameters
    ----------
    installation_uid : str
        Blackwell UID to include in the query.

    Returns
    -------
    update_information : dict
        Updates information.
    """
    current_version = f'v{blackwell.__version__}'
    headers = {
        BLACKWELL_UID_KEY: installation_uid,
        BLACKWELL_TAG_KEY: current_version,
        USER_AGENT_KEY: f'Blackwell UpdateCheckService/{current_version}'
    }

    update_information = get_update_information_template(
        uuid=installation_uid,
        update_check=True,
        current_version=current_version,
        last_check_date=get_utc_now()
    )

    async with httpx.AsyncClient(verify=_get_ssl_context()) as client:
        try:
            response = await client.get(RELEASE_UPDATES_URL, headers=headers, follow_redirects=True)
            response_data = response.json()

            update_information['status_code'] = response.status_code

            if response.status_code == 200:
                if len(response_data['data']['major']):
                    update_information['last_available_major'].update(**response_data['data']['major'][-1])
                if len(response_data['data']['minor']):
                    update_information['last_available_minor'].update(**response_data['data']['minor'][-1])
                if len(response_data['data']['patch']):
                    update_information['last_available_patch'].update(**response_data['data']['patch'][-1])
            else:
                update_information['message'] = response_data['errors']['detail']
        except httpx.RequestError as err:
            update_information.update({'message': str(err), 'status_code': 500})
        except Exception as err:
            update_information.update({'message': str(err), 'status_code': 500})

    return update_information
