#include "ui/ChannelView.Selector.hpp"

namespace chatterino::ui
{
    namespace
    {
        QString str(const SelectionItem& i)
        {
            return QString::number(i.messageIndex) + ":" +
                   QString::number(i.charIndex);
        }
    }  // namespace

    Selection Selector::selection()
    {
        return this->selection_;
    }

    void Selector::start(const SelectionItem& pos,
        const SelectionItem& wordStart, const SelectionItem& wordEnd)
    {
        this->startRegular_ = pos;
        this->startWordLeft_ = wordStart;
        this->startWordRight_ = wordEnd;

        this->moveRegular(pos);
    }

    void Selector::moveRegular(const SelectionItem& pos)
    {
        this->selection_ = {this->startRegular_, pos};
    }

    void Selector::moveWord(
        const SelectionItem& start, const SelectionItem& end)
    {
        if (start < this->startWordLeft_)
            this->selection_ = {start, this->startWordRight_};
        else
            this->selection_ = {this->startWordLeft_, end};
    }

    void Selector::clear()
    {
        this->selection_ = {};
    }
}  // namespace chatterino::ui
