#pragma once

#include "ab/BaseWidget.hpp"
#include "ui/FlexLayout.Private.hpp"
#include "ui/UiFwd.hpp"

namespace chatterino::ui
{
    class FlexLayout;

    class SplitContainer : public ab::BaseWidget
    {
    public:
        SplitContainer(Application& app);

        void deserialize(const QJsonObject& obj, Application& app);

    private:
        void decodeNodeRecusively(QJsonObject& obj, QObject* node);

        Application& app_;
        FlexLayout* flex_{};
    };
}  // namespace chatterino::ui
