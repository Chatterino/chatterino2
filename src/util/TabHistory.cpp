// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/TabHistory.hpp"

namespace chatterino {

void TabHistory::recordVisit(QWidget *from)
{
    if (from == nullptr)
    {
        return;
    }

    this->history_.erase(
        this->history_.begin() + static_cast<std::ptrdiff_t>(this->backCount_),
        this->history_.end());
    this->history_.push_back(from);
    this->backCount_++;

    while (this->history_.size() > MAX_TAB_HISTORY_SIZE)
    {
        this->history_.pop_front();
        if (this->backCount_ > 0)
        {
            this->backCount_--;
        }
    }
}

std::optional<QWidget *> TabHistory::goBack(QWidget *current)
{
    if (this->backCount_ == 0)
    {
        return std::nullopt;
    }

    QWidget *target = this->history_[this->backCount_ - 1];
    this->history_.erase(this->history_.begin() +
                         static_cast<std::ptrdiff_t>(this->backCount_ - 1));
    this->backCount_--;

    if (current != nullptr)
    {
        this->history_.push_back(current);
    }

    return target;
}

std::optional<QWidget *> TabHistory::goForward(QWidget *current)
{
    if (this->backCount_ >= this->history_.size())
    {
        return std::nullopt;
    }

    QWidget *target = this->history_.back();
    this->history_.pop_back();

    if (current != nullptr)
    {
        this->history_.insert(this->history_.begin() +
                                  static_cast<std::ptrdiff_t>(this->backCount_),
                              current);
        this->backCount_++;
    }

    return target;
}

void TabHistory::removePage(QWidget *page)
{
    for (size_t i = 0; i < this->history_.size();)
    {
        if (this->history_[i] == page)
        {
            this->history_.erase(this->history_.begin() +
                                 static_cast<std::ptrdiff_t>(i));
            if (i < this->backCount_)
            {
                this->backCount_--;
            }
        }
        else
        {
            ++i;
        }
    }
}

bool TabHistory::canGoBack() const
{
    return this->backCount_ > 0;
}

bool TabHistory::canGoForward() const
{
    return this->backCount_ < this->history_.size();
}

std::optional<QWidget *> TabHistory::peekBack() const
{
    if (this->backCount_ == 0)
    {
        return std::nullopt;
    }

    return this->history_[this->backCount_ - 1];
}

std::optional<QWidget *> TabHistory::peekForward() const
{
    if (this->backCount_ >= this->history_.size())
    {
        return std::nullopt;
    }

    return this->history_.back();
}

std::vector<QWidget *> TabHistory::backStackMostRecentFirst() const
{
    std::vector<QWidget *> result;
    result.reserve(this->backCount_);

    for (size_t i = this->backCount_; i > 0; --i)
    {
        result.push_back(this->history_[i - 1]);
    }

    return result;
}

void TabHistory::discardBackTop()
{
    if (this->backCount_ == 0)
    {
        return;
    }

    this->history_.erase(this->history_.begin() +
                         static_cast<std::ptrdiff_t>(this->backCount_ - 1));
    this->backCount_--;
}

void TabHistory::discardForwardTop()
{
    if (this->backCount_ >= this->history_.size())
    {
        return;
    }

    this->history_.pop_back();
}

}  // namespace chatterino
