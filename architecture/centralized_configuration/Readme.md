<!---
Copyright (C) 2015, Blackwell Inc.
Created by Blackwell, Inc. <info@blackwell.com>.
This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
-->

# Centralized Configuration
## Index
- [Centralized Configuration](#centralized-configuration)
  - [Index](#index)
  - [Purpose](#purpose)
  - [Sequence diagram](#sequence-diagram)

## Purpose

One of the key features of Blackwell as a EDR is the Centralized Configuration, allowing to deploy configurations, policies, rootcheck descriptions or any other file from Blackwell Manager to any Blackwell Agent based on their grouping configuration. This feature has multiples actors: Blackwell Cluster (Master and Worker nodes), with `blackwell-remoted` as the main responsible from the managment side, and Blackwell Agent with `blackwell-agentd` as resposible from the client side.


## Sequence diagram
Sequence diagram shows the basic flow of Centralized Configuration based on the configuration provided. There are mainly three stages:
1. Blackwell Manager Master Node (`blackwell-remoted`) creates every `remoted.shared_reload` (internal) seconds the files that need to be synchronized with the agents.
2. Blackwell Cluster as a whole (via `blackwell-clusterd`) continuously synchronize files between Blackwell Manager Master Node and Blackwell Manager Worker Nodes
3. Blackwell Agent `blackwell-agentd` (via ) sends every `notify_time` (ossec.conf) their status, being `merged.mg` hash part of it. Blackwell Manager Worker Node (`blackwell-remoted`) will check if agent's `merged.mg` is out-of-date, and in case this is true, the new `merged.mg` will be pushed to Blackwell Agent.
