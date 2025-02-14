<!---
Copyright (C) 2015, Blackwell Inc.
Created by Blackwell, Inc. <info@blackwell.com>.
This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
-->

# Metrics

## Index

- [Metrics](#metrics)
  - [Index](#index)
  - [Purpose](#purpose)
  - [Sequence diagram](#sequence-diagram)

## Purpose

Blackwell includes some metrics to understand the behavior of its components, which allow to investigate errors and detect problems with some configurations. This feature has multiple actors: `blackwell-remoted` for agent interaction messages, `blackwell-analysisd` for processed events.

## Sequence diagram

The sequence diagram shows the basic flow of metric counters. These are the main flows:

1. Messages received by `blackwell-remoted` from agents.
2. Messages that `blackwell-remoted` sends to agents.
3. Events received by `blackwell-analysisd`.
4. Events processed by `blackwell-analysisd`.
