'''
copyright: Copyright (C) 2015-2024, Blackwell Inc.

           Created by Blackwell, Inc. <info@blackwell.com>.

           This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

type: integration

brief: The 'blackwell-logcollector' daemon monitors configured files and commands for new log messages.
       Specifically, these tests will check if the Blackwell component (agent or manager) starts when
       the 'label' tag is set in the configuration, and the Blackwell API returns the same values for
       the configured 'localfile' section.
       Log data collection is the real-time process of making sense out of the records generated by
       servers or devices. This component can receive logs through text files or Windows event logs.
       It can also directly receive logs via remote syslog which is useful for firewalls and
       other such devices.

components:
    - logcollector

suite: configuration

targets:
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
    - https://documentation.blackwell.com/current/user-manual/reference/ossec-conf/localfile.html#label

tags:
    - logcollector_configuration
'''

import pytest, sys, tempfile, re, os

from pathlib import Path

from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH
from blackwell_testing.constants.platforms import WINDOWS
from blackwell_testing.constants.daemons import LOGCOLLECTOR_DAEMON
from blackwell_testing.modules.logcollector import patterns
from blackwell_testing.modules.logcollector import utils
from blackwell_testing.tools.monitors import file_monitor
from blackwell_testing.utils import callbacks, configuration
from blackwell_testing.utils.services import control_service
from blackwell_testing.utils.file import truncate_file

from . import TEST_CASES_PATH, CONFIGURATIONS_PATH


# Marks
pytestmark = [pytest.mark.agent, pytest.mark.linux, pytest.mark.win32, pytest.mark.darwin, pytest.mark.tier(level=0)]

# Configuration
cases_path = Path(TEST_CASES_PATH, 'cases_basic_configuration_label.yaml')
config_path = Path(CONFIGURATIONS_PATH, 'blackwell_basic_configuration_label.yaml')

test_configuration, test_metadata, test_cases_ids = configuration.get_test_cases_data(cases_path)

folder_path = tempfile.gettempdir()
location = os.path.join(folder_path, 'test.txt')
for test in test_metadata:
    if test['location']:
        test['location'] = re.escape(location)
for test in test_configuration:
    if test['LOCATION']:
        test['LOCATION'] = location

test_configuration = configuration.load_configuration_template(config_path, test_configuration, test_metadata)

# Test daemons to restart.
daemons_handler_configuration = {'all_daemons': True}


@pytest.mark.parametrize('test_configuration, test_metadata', zip(test_configuration, test_metadata), ids=test_cases_ids)
def test_configuration_label(test_configuration, test_metadata, set_blackwell_configuration, daemons_handler_module, stop_logcollector):
    '''
    description: Check if the 'blackwell-logcollector' daemon can monitor log files configured to use labels.
                 For this purpose, the test will configure the logcollector to use labels, setting them
                 in the label 'tag'. Once the logcollector has started, it will check if the 'analyzing'
                 event, indicating that the testing log file is being monitored, has been generated.
                 Finally, the test will verify that the Blackwell API returns the same values for
                 the 'localfile' section that the configured one.

    blackwell_min_version: 4.2.0

    tier: 0

    parameters:
        - test_configuration:
            type: data
            brief: Configuration used in the test.
        - test_metadata:
            type: data
            brief: Configuration cases.
        - set_blackwell_configuration:
            type: fixture
            brief: Configure a custom environment for testing.
        - daemons_handler_module:
            type: fixture
            brief: Handler of Blackwell daemons.
        - stop_logcollector:
            type: fixture
            brief: Stop logcollector daemon.

    assertions:
        - Verify that the logcollector monitors files when using the 'label' tag.
        - Verify that the Blackwell API returns the same values for the 'localfile' section as the configured one.

    input_description: A configuration template (test_basic_configuration_label) is contained in an external
                       YAML file (blackwell_basic_configuration.yaml). That template is combined with different
                       test cases defined in the module. Those include configuration settings for
                       the 'blackwell-logcollector' daemon.

    expected_output:
        - r'Analyzing file.*'

    tags:
        - invalid_settings
        - logs
    '''

    control_service('start', daemon=LOGCOLLECTOR_DAEMON)

    # Wait for command
    blackwell_log_monitor = file_monitor.FileMonitor(BLACKWELL_LOG_PATH)
    blackwell_log_monitor.start(callback=callbacks.generate_callback(patterns.LOGCOLLECTOR_ANALYZING_FILE, {
                              'file': test_metadata['location']
                          }), timeout=10)
    assert (blackwell_log_monitor.callback_result != None), patterns.ERROR_ANALYZING_FILE

    if sys.platform != WINDOWS:
        utils.check_logcollector_socket()
        utils.validate_test_config_with_module_config(test_configuration=test_configuration)
