# Copyright (C) 2015, Blackwell Inc.
# Created by Blackwell, Inc. <info@blackwell.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import json
from unittest.mock import patch

import pytest

with patch('blackwell.common.blackwell_uid'):
    with patch('blackwell.common.blackwell_gid'):
        from api.encoder import prettify, dumps
        from blackwell.core.results import BlackwellResult


def custom_hook(dct):
    if 'key' in dct:
        return {'key': dct['key']}
    elif 'error' in dct:
        return BlackwellResult.decode_json({'result': dct, 'str_priority': 'v2'})
    else:
        return dct


@pytest.mark.parametrize('o', [{'key': 'v1'},
                               BlackwellResult({'k1': 'v1'}, str_priority='v2')
                               ]
                         )
def test_encoder_dumps(o):
    """Test dumps method from API encoder using BlackwellAPIJSONEncoder."""
    encoded = dumps(o)
    decoded = json.loads(encoded, object_hook=custom_hook)
    assert decoded == o


def test_encoder_prettify():
    """Test prettify method from API encoder using BlackwellAPIJSONEncoder."""
    assert prettify({'k1': 'v1'}) == '{\n   "k1": "v1"\n}'
