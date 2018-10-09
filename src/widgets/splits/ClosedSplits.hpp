#ifndef CLOSEDSPLITS_HPP
#define CLOSEDSPLITS_HPP

#include "common/Channel.hpp"
#include "widgets/helper/NotebookTab.hpp"

#include <deque>
#include <mutex>
#include <utility>

namespace chatterino {

class ClosedSplits
{
public:
    struct SplitInfo {
        QString channelName;
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

#endif  // CLOSEDSPLITS_HPP
