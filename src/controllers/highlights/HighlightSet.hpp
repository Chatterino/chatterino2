#pragma once

#include "controllers/highlights/HighlightPhrase.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class HighlightSet
{
public:
    HighlightSet()
    {
        this->listener_ =
            getCSettings().highlightedMessages.delayedItemsChanged.connect([this] {
                    this->reloadHighlights();
            });
    }

    HighlightSet(const QList<HighlightDescriptor> &highlights)
    {
        auto messageHighlights = getCSettings().highlightedMessages.readOnly();
        for (const auto &h : *messageHighlights)
        {
            for (const auto &descriptor : highlights) {
                if (descriptor.id_ == h->getId())
                    this->highlights_.insert(h->getId(), h);
            }
        }

        this->listener_ =
            getCSettings().filterRecords.delayedItemsChanged.connect([this] {
                this->reloadHighlights();
            });
    }

    ~HighlightSet()
    {
        this->listener_.disconnect();
    }

private:
    QMap<QString, HighlightPhrasePtr> highlights_;
    pajlada::Signals::Connection listener_;

    void reloadHighlights()
    {
        auto highlights = getCSettings().highlightedMessages.readOnly();
        for (const auto &key : this->highlights_.keys())
        {
            bool found = false;
            for (const auto &h : *highlights)
            {
                if (h->getId() == key)
                {
                    found = true;
                    this->highlights_.insert(key, h);
                }
            }
            if (!found)
            {
                this->highlights_.remove(key);
            }
        }
    }
};

using HighlightSetPtr = std::shared_ptr<HighlightSet>;

}  // namespace chatterino
