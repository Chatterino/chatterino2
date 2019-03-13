#pragma once

#include "ab/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino
{
    class MessageContainer;
}

namespace chatterino::ui
{
    class MessageView;

    class SearchWindow : public ab::BaseWindow
    {
    public:
        SearchWindow();

        void setMessages(std::shared_ptr<MessageContainer> messages);

    private:
        void initLayout();
        void performSearch();

        QLineEdit* searchInput_{};
        MessageView* messageView_{};
        std::shared_ptr<MessageContainer> messages_;
    };
}  // namespace chatterino::ui
