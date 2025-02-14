# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import sys
from unittest.mock import MagicMock, patch

import pytest
from connexion.lifecycle import ConnexionResponse

with patch('blackwell.common.blackwell_uid'):
    with patch('blackwell.common.blackwell_gid'):
        sys.modules['blackwell.rbac.orm'] = MagicMock()
        import blackwell.rbac.decorators
        from api.controllers.default_controller import (BasicInfo, DATE_FORMAT,
                                                        default_info, socket)
        from blackwell.tests.util import RBAC_bypasser
        from blackwell.core.utils import get_utc_now
        blackwell.rbac.decorators.expose_resources = RBAC_bypasser
        del sys.modules['blackwell.rbac.orm']


@pytest.mark.asyncio
@patch('api.controllers.default_controller.load_spec', return_value=MagicMock())
@patch('api.controllers.default_controller.BlackwellResult', return_value={})
async def test_default_info(mock_wresult, mock_lspec):
    """Verify 'default_info' endpoint is working as expected."""
    result = await default_info()
    data = {
        'title': mock_lspec.return_value['info']['title'],
        'api_version': mock_lspec.return_value['info']['version'],
        'revision': mock_lspec.return_value['info']['x-revision'],
        'license_name': mock_lspec.return_value['info']['license']['name'],
        'license_url': mock_lspec.return_value['info']['license']['url'],
        'hostname': socket.gethostname(),
        'timestamp': get_utc_now().strftime(DATE_FORMAT)
    }
    mock_lspec.assert_called_once_with()
    mock_wresult.assert_called_once_with({'data': BasicInfo.from_dict(data)})
    assert isinstance(result, ConnexionResponse)
