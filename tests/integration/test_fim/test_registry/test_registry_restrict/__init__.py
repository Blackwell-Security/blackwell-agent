# Copyright (C) 2015-2024, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
from pathlib import Path


# Constants & base paths
DATA_PATH = Path(Path(__file__).parent, 'data')
CONFIGS_PATH = Path(DATA_PATH, 'configuration_templates')
TEST_CASES_PATH = Path(DATA_PATH, 'test_cases')
