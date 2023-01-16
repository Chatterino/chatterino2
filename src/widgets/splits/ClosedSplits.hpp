#pragma once

#include <QList>
#include <QUuid>

#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace chatterino {

class NotebookTab;

class ClosedSplits
{
public:
    struct SplitInfo {
        QString channelName;
        QList<QUuid> filters;
        NotebookTab *tab;  // non owning ptr
    };

    static void invalidateTab(NotebookTab *const tab);
    static void push(const SplitInfo &si);
    static void push(SplitInfo &&si);
    static SplitInfo pop();
    static bool empty();
    static std::size_t size();

private:
    static std::mutex m_;
    static std::vector<SplitInfo> closedSplits_;
};

}  // namespace chatterino
