# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import sys
from unittest.mock import ANY, AsyncMock, MagicMock, patch

import pytest
from connexion.lifecycle import ConnexionResponse

from api.controllers.test.utils import CustomAffectedItems

with patch('blackwell.common.blackwell_uid'):
    with patch('blackwell.common.blackwell_gid'):
        sys.modules['blackwell.rbac.orm'] = MagicMock()
        import blackwell.rbac.decorators
        from blackwell.event import send_event_to_analysisd
        from blackwell.tests.util import RBAC_bypasser

        from api.controllers.event_controller import forward_event

        blackwell.rbac.decorators.expose_resources = RBAC_bypasser
        del sys.modules['blackwell.rbac.orm']


@pytest.mark.asyncio
@pytest.mark.parametrize("mock_request", ["event_controller"], indirect=True)
@patch('api.configuration.api_conf')
@patch('api.controllers.event_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.event_controller.remove_nones_to_dict')
@patch('api.controllers.event_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.event_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_forward_event(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_exp, mock_request):
    """Verify 'forward_event' endpoint is working as expected."""
    with patch('api.controllers.event_controller.Body.validate_content_type'):
        with patch(
            'api.controllers.event_controller.EventIngestModel.get_kwargs', return_value=AsyncMock()
        ) as mock_getkwargs:

            result = await forward_event()
            mock_dapi.assert_called_once_with(
                f=send_event_to_analysisd,
                f_kwargs=mock_remove.return_value,
                request_type='local_any',
                is_async=False,
                wait_for_complete=False,
                logger=ANY,
                rbac_permissions=mock_request.context['token_info']['rbac_policies']
            )
            mock_exc.assert_called_once_with(mock_dfunc.return_value)
            mock_remove.assert_called_once_with(mock_getkwargs.return_value)
            assert isinstance(result, ConnexionResponse)
