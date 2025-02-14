'''
copyright: Copyright (C) 2015-2024, Blackwell Inc.
           Created by Blackwell, Inc. <info@blackwell.com>.
           This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
type: integration
brief: Blackwell-db is the daemon in charge of the databases with all the Blackwell persistent information, exposing a socket
       to receive requests and provide information. The Blackwell core uses list-based databases to store information
       related to agent keys, and FIM/Rootcheck event data.
       This test checks the usage of the blackwelldb getconfig command used to get the current configuration

tier: 0

modules:
    - blackwell_db

components:
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
    - CentOS 6
    - Ubuntu Focal
    - Ubuntu Bionic
    - Ubuntu Xenial
    - Ubuntu Trusty
    - Debian Buster
    - Debian Stretch
    - Debian Jessie
    - Debian Wheezy
    - Red Hat 8
    - Red Hat 7
    - Red Hat 6

references:
    - https://documentation.blackwell.com/current/user-manual/reference/daemons/blackwell-db.html

tags:
    - blackwell_db
'''
from pathlib import Path
import pytest
from blackwell_testing.constants.paths.sockets import BLACKWELL_DB_SOCKET_PATH
from blackwell_testing.constants.daemons import BLACKWELL_DB_DAEMON
from blackwell_testing.utils.database import query_wdb
from blackwell_testing.utils import configuration

from . import TEST_CASES_FOLDER_PATH

# Marks
pytestmark = [pytest.mark.server, pytest.mark.tier(level=0)]

# Configurations
t_cases_path = Path(TEST_CASES_FOLDER_PATH, 'cases_blackwelldb_getconfig.yaml')
t_config_parameters, t_config_metadata, t_case_ids = configuration.get_test_cases_data(t_cases_path)

receiver_sockets_params = [(BLACKWELL_DB_SOCKET_PATH, 'AF_UNIX', 'TCP')]
monitored_sockets_params = [(BLACKWELL_DB_DAEMON, None, True)]
receiver_sockets = None  # Set in the fixtures


# Tests
@pytest.mark.parametrize('test_metadata', t_config_metadata, ids=t_case_ids)
def test_sync_agent_groups(configure_sockets_environment_module, connect_to_sockets_module, test_metadata):
    '''
    description: Check that commands about blackwelldb getconfig works properly.
    blackwell_min_version: 4.4.0
    parameters:
        - configure_sockets_environment_module:
            type: fixture
            brief: Configure environment for sockets and MITM.
        - connect_to_sockets_module:
            type: fixture
            brief: Module scope version of 'connect_to_sockets' fixture.
        - test_metadata:
            type: dict
            brief: Test case metadata.
    assertions:
        - Verify that the socket response matches the expected output.
    input_description:
        - Test cases are defined in the blackwelldb_getconfig.yaml file.
    expected_output:
        - an array with the configuration of DB.
    tags:
        - blackwell_db
        - wdb_socket
    '''
    # Set each case
    output = test_metadata["output"]

    response = query_wdb(test_metadata["input"])

    # Validate response
    assert output in str(response), f"The expected output: {output} was not found in response: {response}"
