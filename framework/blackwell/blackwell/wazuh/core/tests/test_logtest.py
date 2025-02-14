# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

from unittest.mock import patch

import pytest

with patch('blackwell.common.blackwell_uid'):
    with patch('blackwell.common.blackwell_gid'):
        from blackwell.core.logtest import send_logtest_msg, validate_dummy_logtest
        from blackwell.core.common import LOGTEST_SOCKET
        from blackwell.core.exception import BlackwellError


@pytest.mark.parametrize('params', [
    {'command': 'random_command', 'parameters': {'param1': 'value1'}},
    {'command': None, 'parameters': None}
])
@patch('blackwell.core.logtest.BlackwellSocketJSON.__init__', return_value=None)
@patch('blackwell.core.logtest.BlackwellSocketJSON.send')
@patch('blackwell.core.logtest.BlackwellSocketJSON.close')
@patch('blackwell.core.logtest.create_blackwell_socket_message')
def test_send_logtest_msg(create_message_mock, close_mock, send_mock, init_mock, params):
    """Test `send_logtest_msg` function from module core.logtest.

    Parameters
    ----------
    params : dict
        Params that will be sent to the logtest socket.
    """
    with patch('blackwell.core.logtest.BlackwellSocketJSON.receive',
               return_value={'data': {'response': True, 'output': {'timestamp': '1970-01-01T00:00:00.000000-0200'}}}):
        response = send_logtest_msg(**params)
        init_mock.assert_called_with(LOGTEST_SOCKET)
        create_message_mock.assert_called_with(origin={'name': 'Logtest', 'module': 'framework'}, **params)
        assert response == {'data': {'response': True, 'output': {'timestamp': '1970-01-01T02:00:00.000000Z'}}}


@patch('blackwell.core.logtest.BlackwellSocketJSON.__init__', return_value=None)
@patch('blackwell.core.logtest.BlackwellSocketJSON.send')
@patch('blackwell.core.logtest.BlackwellSocketJSON.close')
@patch('blackwell.core.logtest.create_blackwell_socket_message')
def test_validate_dummy_logtest(create_message_mock, close_mock, send_mock, init_mock):
    with patch('blackwell.core.logtest.BlackwellSocketJSON.receive',
               return_value={'data': {'codemsg': -1}, 'error': 0}):
        with pytest.raises(BlackwellError) as err_info:
            validate_dummy_logtest()

        assert err_info.value.code == 1113
