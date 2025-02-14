"""
 Copyright (C) 2015-2024, Blackwell Inc.
 Created by Blackwell, Inc. <info@blackwell.com>.
 This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
"""

import pytest

from pathlib import Path
from blackwell_testing.constants.paths.configurations import BLACKWELL_CONF_PATH
from blackwell_testing.tools.monitors.file_monitor import FileMonitor
from blackwell_testing.utils.callbacks import generate_callback
from blackwell_testing.utils.configuration import get_test_cases_data, load_configuration_template
from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH

from . import CONFIGS_PATH, TEST_CASES_PATH

from blackwell_testing.modules.remoted.configuration import REMOTED_DEBUG
from blackwell_testing.modules.remoted import patterns
from blackwell_testing.modules.api import utils

# Set pytest marks.
pytestmark = [pytest.mark.server, pytest.mark.tier(level=1)]

# Cases metadata and its ids.
cases_path = Path(TEST_CASES_PATH, 'cases_valid_queue_size.yaml')
config_path = Path(CONFIGS_PATH, 'config_queue_size.yaml')
test_configuration, test_metadata, cases_ids = get_test_cases_data(cases_path)
test_configuration = load_configuration_template(config_path, test_configuration, test_metadata)

daemons_handler_configuration = {'all_daemons': True}

local_internal_options = {REMOTED_DEBUG: '2'}

# Test function.
@pytest.mark.parametrize('test_configuration, test_metadata',  zip(test_configuration, test_metadata), ids=cases_ids)
def test_queue_size_syslog(test_configuration, test_metadata, configure_local_internal_options, truncate_monitored_files,
                            set_blackwell_configuration, restart_blackwell_expect_error, get_real_configuration):

    '''
    description: Check that when 'blackwell-remoted' sets a valid queue size. For this purpose, it uses the configuration
                 from test cases, check if the warning has been logged and the configuration is the same as the API
                 response.

    parameters:
        - test_configuration
            type: dict
            brief: Configuration applied to ossec.conf.
        - test_metadata:
            type: dict
            brief: Test case metadata.
        - truncate_monitored_files:
            type: fixture
            brief: Truncate all the log files and json alerts files before and after the test execution.
        - configure_local_internal_options:
            type: fixture
            brief: Configure the Blackwell local internal options using the values from `local_internal_options`.
        - daemons_handler:
            type: fixture
            brief: Starts/Restarts the daemons indicated in `daemons_handler_configuration` before each test,
                   once the test finishes, stops the daemons.
        - restart_blackwell_expect_error
            type: fixture
            brief: Restart service when expected error is None, once the test finishes stops the daemons.
        - get_real_configuration
            type: fixture
            brief: get elements from section config and convert  list to dict
    '''

    log_monitor = FileMonitor(BLACKWELL_LOG_PATH)

    real_config_list = get_real_configuration
    utils.compare_config_api_response(real_config_list, 'remote')
