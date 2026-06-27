// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/TabHistory.hpp"

namespace chatterino {

void TabHistory::recordNavigation(QWidget *from, QWidget *to)
{
    if (to == nullptr)
    {
        return;
    }

    if (!this->history_.empty())
    {
        this->history_.erase(
            this->history_.begin() +
                static_cast<std::ptrdiff_t>(this->currentIndex_ + 1),
            this->history_.end());
    }

    if (this->history_.empty())
    {
        if (from != nullptr)
        {
            this->history_.push_back(from);
        }
    }

    this->history_.push_back(to);
    this->currentIndex_ = this->history_.size() - 1;

    while (this->history_.size() > MAX_TAB_HISTORY_SIZE)
    {
        if (this->currentIndex_ > 0)
        {
            this->history_.pop_front();
            this->currentIndex_--;
        }
        else if (this->currentIndex_ + 1 < this->history_.size())
        {
            this->history_.pop_back();
        }
        else
        {
            break;
        }
    }
}

std::optional<QWidget *> TabHistory::goBack()
{
    if (this->currentIndex_ == 0)
    {
        return std::nullopt;
    }

    this->currentIndex_--;
    return this->history_[this->currentIndex_];
}

std::optional<QWidget *> TabHistory::goForward()
{
    if (this->currentIndex_ + 1 >= this->history_.size())
    {
        return std::nullopt;
    }

    this->currentIndex_++;
    return this->history_[this->currentIndex_];
}

void TabHistory::removePage(QWidget *page)
{
    for (size_t i = 0; i < this->history_.size();)
    {
        if (this->history_[i] == page)
        {
            this->history_.erase(this->history_.begin() +
                                 static_cast<std::ptrdiff_t>(i));
            if (i < this->currentIndex_)
            {
                this->currentIndex_--;
            }
            else if (i == this->currentIndex_)
            {
                if (this->currentIndex_ >= this->history_.size())
                {
                    if (this->history_.empty())
                    {
                        this->currentIndex_ = 0;
                    }
                    else
                    {
                        this->currentIndex_ = this->history_.size() - 1;
                    }
                }
                else if (this->currentIndex_ > 0)
                {
                    this->currentIndex_--;
                }
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
    return this->currentIndex_ > 0;
}

bool TabHistory::canGoForward() const
{
    return this->currentIndex_ + 1 < this->history_.size();
}

std::optional<QWidget *> TabHistory::peekBack() const
{
    if (this->currentIndex_ == 0)
    {
        return std::nullopt;
    }

    return this->history_[this->currentIndex_ - 1];
}

std::optional<QWidget *> TabHistory::peekForward() const
{
    if (this->currentIndex_ + 1 >= this->history_.size())
    {
        return std::nullopt;
    }

    return this->history_[this->currentIndex_ + 1];
}

std::vector<QWidget *> TabHistory::backStackMostRecentFirst() const
{
    std::vector<QWidget *> result;
    result.reserve(this->currentIndex_);

    for (size_t i = this->currentIndex_; i > 0; --i)
    {
        result.push_back(this->history_[i - 1]);
    }

    return result;
}

void TabHistory::discardBackTop()
{
    if (this->currentIndex_ == 0)
    {
        return;
    }

    this->history_.erase(this->history_.begin() +
                         static_cast<std::ptrdiff_t>(this->currentIndex_ - 1));
    this->currentIndex_--;
}

void TabHistory::discardForwardTop()
{
    if (this->currentIndex_ + 1 >= this->history_.size())
    {
        return;
    }

    this->history_.erase(this->history_.begin() +
                         static_cast<std::ptrdiff_t>(this->currentIndex_ + 1));
}

}  // namespace chatterino
