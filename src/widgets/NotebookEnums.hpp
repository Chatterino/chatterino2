#pragma once
#include "common/FlagsEnum.hpp"

namespace chatterino {

enum NotebookTabLocation { Top = 0, Left = 1, Right = 2, Bottom = 3 };

// Controls the visibility of tabs in this notebook
enum NotebookTabVisibilityFlag : uint8_t {
    // Show all tabs
    AllTabs = 1 << 0,

    // Only show tabs containing splits that are live
    Live = 1 << 1,

    // Only show tabs containing splits that have unread messages
    Unread = 1 << 2,
};
using NotebookTabVisibilityFlags = FlagsEnum<NotebookTabVisibilityFlag>;

}  // namespace chatterino
