'''
copyright: Copyright (C) 2015-2024, Blackwell Inc.
           Created by Blackwell, Inc. <info@blackwell.com>.
           This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
'''
import pytest

from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH
from blackwell_testing.modules.integratord.patterns import INTEGRATORD_CONNECTED
from blackwell_testing.tools.monitors.file_monitor import FileMonitor
from blackwell_testing.utils import callbacks


@pytest.fixture()
def wait_for_integratord_start(request):
    # Wait for integratord thread to start
    log_monitor = FileMonitor(BLACKWELL_LOG_PATH)
    log_monitor.start(callback=callbacks.generate_callback(INTEGRATORD_CONNECTED))
    assert (log_monitor.callback_result == None), f'Error invalid configuration event not detected'
