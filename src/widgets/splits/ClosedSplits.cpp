#include "widgets/splits/ClosedSplits.hpp"

namespace chatterino {

std::mutex ClosedSplits::m_;
std::vector<ClosedSplits::SplitInfo> ClosedSplits::closedSplits_;

void ClosedSplits::invalidateTab(NotebookTab *const tab)
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    auto it = std::find_if(ClosedSplits::closedSplits_.begin(),
                           ClosedSplits::closedSplits_.end(),
                           [tab](const auto &item) -> bool {
                               return item.tab == tab;
                           });
    if (it == ClosedSplits::closedSplits_.end())
    {
        return;
    }
    it->tab = nullptr;
}

void ClosedSplits::push(const SplitInfo &si)
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    ClosedSplits::closedSplits_.push_back(si);
}

void ClosedSplits::push(SplitInfo &&si)
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    ClosedSplits::closedSplits_.push_back(std::move(si));
}

ClosedSplits::SplitInfo ClosedSplits::pop()
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    SplitInfo si = std::move(ClosedSplits::closedSplits_.back());
    ClosedSplits::closedSplits_.pop_back();
    return si;
}

bool ClosedSplits::empty()
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    return ClosedSplits::closedSplits_.empty();
}

std::size_t ClosedSplits::size()
{
    std::lock_guard<std::mutex> lk(ClosedSplits::m_);
    return ClosedSplits::closedSplits_.size();
}

}  // namespace chatterino
