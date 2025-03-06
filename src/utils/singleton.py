#
# Copyright © 2020-2025 NVIDIA CORPORATION & AFFILIATES. ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of Nvidia Corporation and its affiliates
# (the "Company") and all right, title, and interest in and to the software
# product, including all associated intellectual property rights, are and
# shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#

# author: Samer Deeb
# date:   Oct 8, 2020
#


class SingletonMeta(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in SingletonMeta._instances:
            single_obj = super(SingletonMeta, cls).__call__(*args, **kwargs)
            SingletonMeta._instances[cls] = single_obj
        return cls._instances[cls]


def _forget_singleton_for_testing(cls):
    # pylint: disable=protected-access
    SingletonMeta._instances.pop(cls, None)
