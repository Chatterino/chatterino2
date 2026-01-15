// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once
#include "common/FlagsEnum.hpp"

namespace chatterino {

enum NotebookTabLocation { Top = 0, Left = 1, Right = 2, Bottom = 3 };

// Controls the visibility of tabs in this notebook
enum NotebookTabVisibilityFlag : uint8_t {
    // All tabs not in the following categories
    Other = 1 << 0,

    // Tabs containing splits that are live
    Live = 1 << 1,

    // Tabs containing splits that are marked unread
    Unread = 1 << 2,
};
using NotebookTabVisibilityFlags = FlagsEnum<NotebookTabVisibilityFlag>;

}  // namespace chatterino
