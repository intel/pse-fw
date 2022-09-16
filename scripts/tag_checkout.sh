#! /bin/bash
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

# This script checks out a specific release tag of PSE codebase after PSE
#code has been fetched using west init and west update.
# The script must be run from the workspace folder (where west init is called)
# Example usage:
#	(workspace)$ ./ehl_pse-fw/scripts/tag_checkout.sh <Tag_Name>

if [ $# -eq 0 ]; then
      echo "No tag provided.. exiting"
      exit 1
fi

cd ehl_pse-fw; git checkout $1; cd -
cd modules/hal/sedi; git checkout $1; cd -
cd zephyr; git checkout $1; cd -
cd zephyr-iotg; git checkout $1; cd -
