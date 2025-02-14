"""
 Copyright (C) 2015-2024, Blackwell Inc.
 Created by Blackwell, Inc. <info@blackwell.com>.
 This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
"""
import pytest
import sys

from blackwell_testing.tools.monitors import file_monitor
from blackwell_testing.modules.modulesd.syscollector import patterns
from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH
from blackwell_testing.utils import callbacks
from blackwell_testing.constants.platforms import WINDOWS


# Fixtures
@pytest.fixture()
def wait_for_syscollector_enabled():
    '''
    Wait for the syscollector module to start.
    '''
    log_monitor = file_monitor.FileMonitor(BLACKWELL_LOG_PATH)
    log_monitor.start(callback=callbacks.generate_callback(patterns.CB_MODULE_STARTED), timeout=60 if sys.platform == WINDOWS else 10)
    assert log_monitor.callback_result
