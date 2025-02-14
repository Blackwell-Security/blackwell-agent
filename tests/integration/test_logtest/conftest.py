# Copyright (C) 2015-2024, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import pytest

from blackwell_testing.modules.analysisd.patterns import LOGTEST_STARTED
from blackwell_testing.tools.monitors.file_monitor import FileMonitor
from blackwell_testing.utils.callbacks import generate_callback
from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH

@pytest.fixture(scope='module')
def wait_for_logtest_startup(request):
    """Wait until logtest has begun."""
    log_monitor = FileMonitor(BLACKWELL_LOG_PATH)
    log_monitor.start(callback=generate_callback(LOGTEST_STARTED), timeout=40, only_new_events=True)
