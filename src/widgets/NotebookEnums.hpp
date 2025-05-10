#pragma once

namespace chatterino {

enum NotebookTabLocation { Top = 0, Left = 1, Right = 2, Bottom = 3 };

// Controls the visibility of tabs in this notebook
enum NotebookTabVisibility : int {
    // Show all tabs
    AllTabs = 0,

    // Only show tabs containing splits that are live
    LiveOnly = 1,
};

}  // namespace chatterino
