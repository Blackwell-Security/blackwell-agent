# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import os
import subprocess
from functools import lru_cache
from sys import exit


@lru_cache(maxsize=None)
def find_blackwell_path() -> str:
    """
    Get the Blackwell installation path.

    Returns
    -------
    str
        Path where Blackwell is installed or empty string if there is no framework in the environment.
    """
    abs_path = os.path.abspath(os.path.dirname(__file__))
    allparts = []
    while 1:
        parts = os.path.split(abs_path)
        if parts[0] == abs_path:  # sentinel for absolute paths
            allparts.insert(0, parts[0])
            break
        elif parts[1] == abs_path:  # sentinel for relative paths
            allparts.insert(0, parts[1])
            break
        else:
            abs_path = parts[0]
            allparts.insert(0, parts[1])

    blackwell_path = ''
    try:
        for i in range(0, allparts.index('wodles')):
            blackwell_path = os.path.join(blackwell_path, allparts[i])
    except ValueError:
        pass

    return blackwell_path


def call_blackwell_control(option: str) -> str:
    """
    Execute the blackwell-control script with the parameters specified.

    Parameters
    ----------
    option : str
        The option that will be passed to the script.

    Returns
    -------
    str
        The output of the call to blackwell-control.
    """
    blackwell_control = os.path.join(find_blackwell_path(), "bin", "blackwell-control")
    try:
        proc = subprocess.Popen([blackwell_control, option], stdout=subprocess.PIPE)
        (stdout, stderr) = proc.communicate()
        return stdout.decode()
    except (OSError, ChildProcessError):
        print(f'ERROR: a problem occurred while executing {blackwell_control}')
        exit(1)


def get_blackwell_info(field: str) -> str:
    """
    Execute the blackwell-control script with the 'info' argument, filtering by field if specified.

    Parameters
    ----------
    field : str
        The field of the output that's being requested. Its value can be 'BLACKWELL_VERSION', 'BLACKWELL_REVISION' or
        'BLACKWELL_TYPE'.

    Returns
    -------
    str
        The output of the blackwell-control script.
    """
    blackwell_info = call_blackwell_control("info")
    if not blackwell_info:
        return "ERROR"

    if not field:
        return blackwell_info

    env_variables = blackwell_info.rsplit("\n")
    env_variables.remove("")
    blackwell_env_vars = dict()
    for env_variable in env_variables:
        key, value = env_variable.split("=")
        blackwell_env_vars[key] = value.replace("\"", "")

    return blackwell_env_vars[field]


@lru_cache(maxsize=None)
def get_blackwell_version() -> str:
    """
    Return the version of Blackwell installed.

    Returns
    -------
    str
        The version of Blackwell installed.
    """
    return get_blackwell_info("BLACKWELL_VERSION")


@lru_cache(maxsize=None)
def get_blackwell_revision() -> str:
    """
    Return the revision of the Blackwell instance installed.

    Returns
    -------
    str
        The revision of the Blackwell instance installed.
    """
    return get_blackwell_info("BLACKWELL_REVISION")


@lru_cache(maxsize=None)
def get_blackwell_type() -> str:
    """
    Return the type of Blackwell instance installed.

    Returns
    -------
    str
        The type of Blackwell instance installed.
    """
    return get_blackwell_info("BLACKWELL_TYPE")


ANALYSISD = os.path.join(find_blackwell_path(), 'queue', 'sockets', 'queue')
# Max size of the event that ANALYSISID can handle
MAX_EVENT_SIZE = 65535
