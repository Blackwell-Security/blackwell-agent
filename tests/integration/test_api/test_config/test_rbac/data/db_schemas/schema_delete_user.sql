/*
 * SQL Schema rbac tests
 * Copyright (C) 2015-2024, Blackwell Inc.
 * Created by Blackwell, Inc. <info@blackwell.com>.
 * This program is a free software, you can redistribute it and/or modify it under the terms of GPLv2.
 */

-- PRAGMA foreign_keys=OFF;
-- BEGIN TRANSACTION;

/* Testing */
DELETE FROM users WHERE username = 'test_user';
