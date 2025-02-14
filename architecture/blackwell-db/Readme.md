<!---
Copyright (C) 2015, Blackwell Inc.
Created by Blackwell, Inc. <info@blackwell.com>.
This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
-->

# Blackwell DB
## Index
- [Blackwell DB](#blackwell-db)
  - [Index](#index)
  - [Purpose](#purpose)
  - [Activity diagrams](#activity-diagrams)


## Purpose
Blackwell DB is a daemon that wraps the access to SQLite database files. It provides:
- Concurrent socket dispatcher.
- Parallel queries to different databases.
- Serialized queries to the same database.
- Dynamic closing of database files.
- Implicit transactions and adjustable committing periods.
- Automatic database upgrades.
- Automatic defragmentation (vacuum) with adjustable parameters.


## Activity diagrams
<dl>
  <dt>001-vacuum</dt><dd>It illustrates the vacuum decision algorithm: in which cases Blackwell DB runs a <code>vacuum</code> command on databases.</dd>
</dl>
