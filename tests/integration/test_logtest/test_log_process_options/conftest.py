# Copyright (C) 2015-2024, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import pytest
from pathlib import Path
import shutil

from blackwell_testing.constants import users
from blackwell_testing.constants.paths.configurations import CUSTOM_RULES_FILE
from . import TEST_RULES_PATH


@pytest.fixture()
def configure_rules_list(test_metadata):
    """Configure a custom rules for testing.

    Restart Blackwell is not needed for applying the configuration, is optional.
    """

    # save current rules
    shutil.copy(CUSTOM_RULES_FILE, CUSTOM_RULES_FILE + '.cpy')

    file_test = Path(TEST_RULES_PATH, test_metadata['rule_file'])
    # copy test rules
    shutil.copy(file_test, CUSTOM_RULES_FILE)
    shutil.chown(CUSTOM_RULES_FILE, users.BLACKWELL_UNIX_USER, users.BLACKWELL_UNIX_GROUP)

    yield

    # restore previous configuration
    shutil.move(CUSTOM_RULES_FILE + '.cpy', CUSTOM_RULES_FILE)
    shutil.chown(CUSTOM_RULES_FILE, users.BLACKWELL_UNIX_USER, users.BLACKWELL_UNIX_GROUP)
