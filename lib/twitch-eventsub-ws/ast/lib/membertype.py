# SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
#
# SPDX-License-Identifier: MIT

from enum import Enum


class MemberType(Enum):
    BASIC = 1
    VECTOR = 2
    OPTIONAL = 3
    OPTIONAL_VECTOR = 4
    VARIANT = 5
