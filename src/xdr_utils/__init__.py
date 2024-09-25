#
# Copyright © 2013-2024 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of Nvidia Corporation and its affiliates
# (the "Company") and all right, title, and interest in and to the software
# product, including all associated intellectual property rights, are and
# shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#

import logging
from typing import List
from enum import Enum


class PortType(Enum):
    LEGACY = '1'
    PLANE = '2'
    AGGREGATED = '4'


PORT_TYPE_NAME_MAP = {
    'legacy': PortType.LEGACY,
    'plane': PortType.PLANE,
    'aggregated': PortType.AGGREGATED
}

SUPPORTED_PORT_TYPES_NAMES = list(PORT_TYPE_NAME_MAP.keys())


def prepare_port_type_http_telemetry_filter(port_types: List[str]) -> str:
    """
       Prepares an HTTP telemetry filter string based on a list of port type names.

       This function takes a list of port type names as input and constructs a filter string
       that can be used in HTTP telemetry requests. Each port type name is mapped to a corresponding
       integer value using the `PORT_TYPE_NAME_MAP` dictionary, which associates string names with
       `PortType` enum members. The resulting filter string is composed by appending the integer values
       to the base filter string 'port_type__in'. If an unsupported port type name is encountered,
       a warning message is logged, and the port type is skipped.

       Parameters:
           port_types (List[str]): A list of strings representing port type names.

       Returns:
           str: A filter string for HTTP telemetry requests that includes the integer values of
                supported port types.
    """
    numeric_port_types = []
    for ptype in port_types:
        ptype_val = PORT_TYPE_NAME_MAP.get(ptype)
        if not ptype_val:
            warn_msg = f'Skipping Port type {ptype}, it should be one of {SUPPORTED_PORT_TYPES_NAMES}'
            logging.warning(warn_msg)
            continue
        numeric_port_types.append(ptype_val.value)
    if not numeric_port_types:
        warn_msg = f'No valid port found in {port_types}'
        logging.warning(warn_msg)
        return ''
    numeric_port_types.sort()
    filter_val = f'port_type__in__{"__".join(numeric_port_types)}'
    return filter_val

if __name__ == '__main__':
    port_type = ['legacy', 'aggregated', 'plane']
    result = prepare_port_type_http_telemetry_filter(port_type)
    print(result)
