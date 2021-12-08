# -*- coding: utf-8 -*-
#
# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
# 

"""
Generates the MQTT username and password

Usage:
    python azure_credentials.py [scope_id] [device_id] [device_key]
Output:
    [username]
    [password]

"""

from base64 import b64encode, b64decode
from hashlib import sha256
import sys
if sys.version_info.major > 2:
    from urllib.parse import quote
else:
    from urllib import quote
from hmac import HMAC
from time import time, sleep
import requests
import json
import sys

import logging
logging.basicConfig(level=logging.ERROR)
logger = logging.getLogger(__name__)

# The default expiration of a SAS token (seconds: one millenium)
AZURE_TOKEN_EXPIRATION = 31556952000
# Endpoint for device provisioning
AZURE_DPS_ENDPOINT = "https://global.azure-devices-provisioning.net"

def main():
    if len(sys.argv) == 4:
        scope_id = sys.argv[1]
        device_id = sys.argv[2]
        device_key = sys.argv[3]
        username, password = _generate_credentials(scope_id, device_id, device_key)
        print("{}\n{}".format(username, password))
    else:
        logger.error("Not enough arguments!")
        logger.error("Usage: python azure_credentials.py [scope_id] [device_id] [device_key]")

def _generate_credentials(scope_id, device_id, device_key):
    """Generate the Azure MQTT username and password
    @param scope_id:   (str) The device's Scope ID
    @param device_id:  (str) The device's Scope ID
    @param device_key: (str) The device's Shared Access Key
    @return: (Tuple[str, str]) The username and password, respectively
    """

    hostname = _retrieve_hostname(scope_id, device_id, device_key)
    username = "{}/{}/?api-version=2018-06-30".format(hostname, device_id)
    password = _generate_sas_token(hostname, device_key)

    return username, password

def _retrieve_hostname(scope_id, device_id, device_key):
    """Retrieve the IoT Central hostname associated to the device
    @param scope_id:   (str) The device's Scope ID
    @param device_id:  (str) The device's ID
    @param device_key: (str) The device's Shared Access Key
    @return:           (str) The IoT Central hostname
    """
    # Get the authentication token for the requests
    expiration = int(time() + 30)
    resource = "{}%2Fregistrations%2F{}".format(scope_id, device_id)
    auth_token = _generate_sas_token(resource, device_key, expiration)
    auth_token += "&skn=registration"

    # Set up the initial HTTP request
    endpoint = "{}/{}/registrations/{}/".format(AZURE_DPS_ENDPOINT, scope_id, device_id)
    registration = "register?api-version=2018-11-01"
    headers = {
        "Accept": "application/json",
        "Content-Type": "application/json; charset=utf-8",
        "Connection": "keep-alive",
        "UserAgent": "prov_device_client/1.0",
        "Authorization": auth_token
    }
    data = {
        "registrationId": device_id
    }

    # Place a registration request for the device (it should already be registered)
    result = requests.put(endpoint + registration, data=json.dumps(data), headers=headers)
    data = json.loads(result.text)

    # Continue checking device's registration status until it resolves
    while data.get("status") == "assigning" and result.ok:
        operation_id = data.get("operationId")
        operation = "operations/{}?api-version=2018-11-01".format(operation_id)

        result = requests.get(endpoint + operation, headers=headers)
        data = json.loads(result.text)

        sleep(1)  # Pause for a bit

    # Get the device's assigned hub
    if not result.ok:
        logger.error("Ran into an error: %s %s", result.status_code, result.text)
    else:
        hub = data.get("registrationState").get("assignedHub")
        logger.debug("Retrieved hostname: %s", hub)
        return hub

def _generate_sas_token(resource, device_key, expiration=None):
    """Create a SAS token for authentication. More information:
    https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security
    @param resource:   (str) The IoT Central resource for which the key is created
    @param device_key: (str) The device's Shared Access Key
    @param expiration: (int) The time at which the token expires
    @return:           (str) The SAS token
    """
    if not expiration:
        expiration = int(time() + AZURE_TOKEN_EXPIRATION)

    sign_key = "{}\n{}".format(resource, expiration)
    signature = b64encode((HMAC(b64decode(device_key.encode("utf-8")), sign_key.encode("utf-8"), sha256).digest()))
    signature = quote(signature)

    return "SharedAccessSignature sr={}&sig={}&se={}".format(
        resource,
        signature,
        expiration
    )

if __name__ == "__main__":
    main()
