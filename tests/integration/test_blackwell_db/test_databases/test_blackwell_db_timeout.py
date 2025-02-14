'''
copyright: Copyright (C) 2015-2024, Blackwell Inc.

           Created by Blackwell, Inc. <info@blackwell.com>.

           This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

type: integration

brief: Blackwell-db is the daemon in charge of the databases with all the Blackwell persistent information, exposing a socket
       to receive requests and provide information. The Blackwell core uses list-based databases to store information
       related to agent keys, and FIM/Rootcheck event data.
       Blackwell-db confirms that is able to save, update and erase the necessary information into the corresponding
       databases, using the proper commands and response strings.

components:
    - blackwell_db

targets:
    - manager

daemons:
    - blackwell-db

os_platform:
    - linux

os_version:
    - Arch Linux
    - Amazon Linux 2
    - Amazon Linux 1
    - CentOS 8
    - CentOS 7
    - Debian Buster
    - Red Hat 8
    - Ubuntu Focal
    - Ubuntu Bionic

references:
    - https://documentation.blackwell.com/current/user-manual/reference/daemons/blackwell-db.html

tags:
    - blackwell_db
'''
import time
import pytest

from blackwell_testing.constants.paths.sockets import BLACKWELL_DB_SOCKET_PATH

# Marks
pytestmark = [pytest.mark.server, pytest.mark.tier(level=0)]

# Variables
receiver_sockets_params = [(BLACKWELL_DB_SOCKET_PATH, 'AF_UNIX', 'TCP')]
BLACKWELL_DB_CHECKSUM_CALCULUS_TIMEOUT = 20

receiver_sockets, monitored_sockets, log_monitors = None, None, None  # Set in the fixtures

# Test daemons to restart.
daemons_handler_configuration = {'all_daemons': True}

# Tests
def test_blackwell_db_timeout(daemons_handler_module, connect_to_sockets_module,
                          pre_insert_packages, pre_set_sync_info):
    """Check that effectively the socket is closed after timeout is reached"""
    blackwell_db_send_sleep = 2
    command = 'agent 000 package get'
    receiver_sockets[0].send(command, size=True)

    # Waiting Blackwell-DB to process command
    time.sleep(blackwell_db_send_sleep)

    socket_closed = False
    cmd_counter = 0
    status = 'due'
    while not socket_closed and status == 'due':
        cmd_counter += 1
        response = receiver_sockets[0].receive(size=True).decode()
        if response == '':
            socket_closed = True
        else:
            status = response.split()[0]

    assert socket_closed, f"Socket never closed. Received {cmd_counter} commands. Last command: {response}"
