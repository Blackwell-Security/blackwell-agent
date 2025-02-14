"""
Copyright (C) 2015-2024, Blackwell Inc.
Created by Blackwell, Inc. <info@blackwell.com>.
This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
"""
import pytest
import os

from blackwell_testing.utils.file import remove_file, recursive_directory_creation
from blackwell_testing.utils.db_queries.global_db import insert_metadata_value, remove_metadata_value


@pytest.fixture()
def remove_backups(request: pytest.FixtureRequest):
    backups_path = getattr(request.module, 'backups_path')
    "Creates backups folder in case it does not exist."
    remove_file(backups_path)
    recursive_directory_creation(backups_path)
    os.chmod(backups_path, 0o777)
    yield
    remove_file(backups_path)
    recursive_directory_creation(backups_path)
    os.chmod(backups_path, 0o777)


@pytest.fixture()
def add_database_values(request):
    test_values = getattr(request.module, 'test_values')
    "Add test values to database"
    insert_metadata_value(test_values[0],test_values[1])
    yield
    remove_metadata_value(test_values[0])
