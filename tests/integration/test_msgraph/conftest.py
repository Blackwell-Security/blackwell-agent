# Copyright (C) 2015-2024, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import pytest
import subprocess
import os
from pathlib import Path

from blackwell_testing.constants.paths.logs import BLACKWELL_LOG_PATH
from blackwell_testing.constants.paths import BLACKWELL_PATH
from blackwell_testing.modules.modulesd import patterns
from blackwell_testing.tools.monitors.file_monitor import FileMonitor
from blackwell_testing.utils import callbacks
from blackwell_testing.utils.file import remove_file


@pytest.fixture()
def wait_for_msgraph_start():
    # Wait for module ms-graph starts
    blackwell_log_monitor = FileMonitor(BLACKWELL_LOG_PATH)
    blackwell_log_monitor.start(callback=callbacks.generate_callback(patterns.MODULESD_STARTED, {
                              'integration': 'ms-graph'
                          }))
    assert (blackwell_log_monitor.callback_result == None), f'Error invalid configuration event not detected'


@pytest.fixture(scope="session")
def proxy_setup():
    RESPONSES_PATH = Path(Path(__file__).parent, 'test_API', 'data', 'response_samples', 'responses.json')
    m365proxy = subprocess.Popen(["/tmp/m365proxy/m365proxy", "--mocks-file", RESPONSES_PATH])
    # Configurate proxy for Blackwell (will only work for systemctl start/restart)
    subprocess.run("systemctl set-environment http_proxy=http://localhost:8000", shell=True)
    remove_file(os.path.join(BLACKWELL_PATH, 'var', 'wodles', 'ms-graph-tenant_id-resource_name-resource_relationship'))

    yield

    subprocess.run("systemctl unset-environment http_proxy", shell=True)
    m365proxy.kill()
    m365proxy.wait()
