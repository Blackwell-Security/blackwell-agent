# # Copyright (C) 2015, Blackwell Inc.
# # Created by Blackwell, Inc. <info@blackwell.com>.
# # This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import logging
import socket
from datetime import datetime

from connexion.lifecycle import ConnexionResponse

from api.controllers.util import json_response
from api.models.basic_info_model import BasicInfo
from blackwell.core.common import DATE_FORMAT
from blackwell.core.results import BlackwellResult
from blackwell.core.security import load_spec
from blackwell.core.utils import get_utc_now

logger = logging.getLogger('blackwell-api')


async def default_info(pretty: bool = False) -> ConnexionResponse:
    """Return basic information about the Blackwell API.

    Parameters
    ----------
    pretty: bool
        Show results in human-readable format.

    Returns
    -------
    ConnexionResponse
        API response.
    """
    info_data = load_spec()
    data = {
        'title': info_data['info']['title'],
        'api_version': info_data['info']['version'],
        'revision': info_data['info']['x-revision'],
        'license_name': info_data['info']['license']['name'],
        'license_url': info_data['info']['license']['url'],
        'hostname': socket.gethostname(),
        'timestamp': get_utc_now().strftime(DATE_FORMAT)
    }
    data = BlackwellResult({'data': BasicInfo.from_dict(data)})

    return json_response(data, pretty=pretty)
