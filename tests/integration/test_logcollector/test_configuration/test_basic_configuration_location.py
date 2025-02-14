'''
copyright: Copyright (C) 2015-2024, Blackwell Inc.

           Created by Blackwell, Inc. <info@blackwell.com>.

           This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

type: integration

brief: The 'blackwell-logcollector' daemon monitors configured files and commands for new log messages.
       Specifically, these tests will check if the Blackwell component (agent or manager) starts when
       the 'location' tag is set in the configuration, and the Blackwell API returns the same values for
       the configured 'localfile' section.
       Log data collection is the real-time process of making sense out of the records generated by
       servers or devices. This component can receive logs through text files or Windows event logs.
       It can also directly receive logs via remote syslog which is useful for firewalls and
       other such devices.

tier: 0

modules:
    - logcollector

components:
    - agent

daemons:
    - blackwell-logcollector
    - blackwell-apid

os_platform:
    - linux
    - macos
    - windows

os_version:
    - Arch Linux
    - Amazon Linux 2
    - Amazon Linux 1
    - CentOS 8
    - CentOS 7
    - Debian Buster
    - Red Hat 8
    - macOS Catalina
    - macOS Server
    - Ubuntu Focal
    - Ubuntu Bionic
    - Windows 10
    - Windows Server 2019
    - Windows Server 2016

references:
    - https://documentation.blackwell.com/current/user-manual/capabilities/log-data-collection/index.html
    - https://documentation.blackwell.com/current/user-manual/reference/ossec-conf/localfile.html#location
    - https://documentation.blackwell.com/current/user-manual/reference/ossec-conf/localfile.html#log-format

tags:
    - logcollector
'''


import pytest, sys, os
import tempfile

from pathlib import Path

from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH
from blackwell_testing.constants.platforms import WINDOWS, MACOS
from blackwell_testing.modules.agentd import configuration as agentd_configuration
from blackwell_testing.modules.logcollector import patterns
from blackwell_testing.modules.logcollector import utils
from blackwell_testing.modules.logcollector import configuration as logcollector_configuration
from blackwell_testing.tools.monitors import file_monitor
from blackwell_testing.utils import callbacks, configuration

from . import TEST_CASES_PATH, CONFIGURATIONS_PATH


LOG_COLLECTOR_GLOBAL_TIMEOUT = 40


# Marks
pytestmark = [pytest.mark.agent, pytest.mark.linux, pytest.mark.win32, pytest.mark.darwin, pytest.mark.tier(level=0)]

# Configuration
default_config_path = Path(CONFIGURATIONS_PATH, 'blackwell_basic_configuration_log_format_location.yaml')
default_cases_path = Path(TEST_CASES_PATH, 'cases_basic_configuration_location.yaml')

macos_cases_path = Path(TEST_CASES_PATH, 'cases_basic_configuration_location_macos.yaml')

win_cases_path = Path(TEST_CASES_PATH, 'cases_basic_configuration_location_win.yaml')

test_configuration, test_metadata, test_cases_ids = configuration.get_test_cases_data(default_cases_path)

folder_path = tempfile.gettempdir()
for test in test_metadata:
    if test['location']:
        location = test['location']
        test['location'] = f'{os.path.join(folder_path, location)}'
for test in test_configuration:
    if test['LOCATION']:
        location = test['LOCATION']
        test['LOCATION'] = f'{os.path.join(folder_path, location)}'

test_configuration = configuration.load_configuration_template(default_config_path, test_configuration, test_metadata)

test_macos_configuration, test_macos_metadata, test_macos_cases_ids = configuration.get_test_cases_data(macos_cases_path)
test_macos_dup_configuration = configuration.load_configuration_template(default_config_path, test_macos_configuration, test_macos_metadata)

test_win_configuration, test_win_metadata, test_win_cases_ids = configuration.get_test_cases_data(win_cases_path)
test_win_configuration = configuration.load_configuration_template(default_config_path, test_win_configuration, test_win_metadata)


daemon_debug = logcollector_configuration.LOGCOLLECTOR_DEBUG

if sys.platform == MACOS:
    test_configuration += test_macos_dup_configuration
    test_metadata += test_macos_metadata
    test_cases_ids += test_macos_cases_ids
if sys.platform == WINDOWS:
    test_configuration += test_win_configuration
    test_metadata += test_win_metadata
    test_cases_ids += test_win_cases_ids
    daemon_debug = agentd_configuration.AGENTD_WINDOWS_DEBUG

# Test daemons to restart.
daemons_handler_configuration = {'all_daemons': True}

local_internal_options = {daemon_debug: '1'}


# Test function.
@pytest.mark.parametrize('test_configuration, test_metadata', zip(test_configuration, test_metadata), ids=test_cases_ids)
def test_configuration_location(test_configuration, test_metadata, truncate_monitored_files, configure_local_internal_options,
                                set_blackwell_configuration, daemons_handler, wait_for_logcollector_start):
    '''
    description: Check if the 'blackwell-logcollector' daemon starts properly when the 'location' tag is used.
                 For this purpose, the test will configure the logcollector to monitor a 'syslog' directory
                 and use a pathname with special characteristics. Finally, the test will verify that the
                 Blackwell component is started by checking its process, and the Blackwell API returns the same
                 values for the 'localfile' section that the configured one.

    blackwell_min_version: 4.2.0

    parameters:
        - test_configuration:
            type: data
            brief: Configuration used in the test.
        - test_metadata:
            type: data
            brief: Configuration cases.
        - truncate_monitored_files:
            type: fixture
            brief: Reset the 'ossec.log' file and start a new monitor.
        - set_blackwell_configuration:
            type: fixture
            brief: Configure a custom environment for testing.
        - daemons_handler:
            type: fixture
            brief: Handler of Blackwell daemons.
        - wait_for_logcollector_start:
            type: fixture
            brief: Wait for the logcollector startup log.

    assertions:
        - Verify that the Blackwell component (agent or manager) can start when the 'location' tag is used.
        - Verify that the Blackwell API returns the same value for the 'localfile' section as the configured one.
        - Verify that the expected event is present in the log file.

    input_description: A configuration template (test_basic_configuration_location) is contained in an external
                       YAML file (blackwell_basic_configuration.yaml). That template is combined with different
                       test cases defined in the module. Those include configuration settings for
                       the 'blackwell-logcollector' daemon.

    expected_output:
        - Boolean values to indicate the state of the Blackwell component.
        - Did not receive the expected "ERROR: Could not EvtSubscribe() for ... which returned ..." event.

    tags:
        - invalid_settings
    '''

    if sys.platform == WINDOWS:
        if test_metadata['location'] == 'invalidchannel':
            blackwell_log_monitor = file_monitor.FileMonitor(BLACKWELL_LOG_PATH)
            callback = callbacks.generate_callback(patterns.LOGCOLLECTOR_EVENTCHANNEL_BAD_FORMAT,
                                                            {'event_location': test_metadata['location']})
            blackwell_log_monitor.start(timeout=LOG_COLLECTOR_GLOBAL_TIMEOUT, callback=callback)
            assert (blackwell_log_monitor.callback_result != None), patterns.ERROR_EVENTCHANNEL
    else:
        if test_metadata['validate_config']:
            utils.check_logcollector_socket()
            if test_metadata['location'] != 'macos' and test_metadata['log_format'] != 'macos':
                utils.validate_test_config_with_module_config(test_configuration=test_configuration)
