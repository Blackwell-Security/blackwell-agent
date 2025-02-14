#!/usr/bin/env python

# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

from blackwell import __version__

from setuptools import setup, find_namespace_packages

setup(name='blackwell',
      version=__version__,
      description='Blackwell control with Python',
      url='https://github.com/wazuh',
      author='Blackwell',
      author_email='hello@blackwell.com',
      license='GPLv2',
      packages=find_namespace_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
      package_data={'blackwell': ['core/blackwell.json',
                              'core/cluster/cluster.json', 'rbac/default/*.yaml']},
      include_package_data=True,
      install_requires=[],
      zip_safe=False,
      )
