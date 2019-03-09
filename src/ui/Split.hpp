#pragma once

#include "Room.hpp"
#include "ab/BaseWidget.hpp"
#include "dialogs/SelectChannelDialog.hpp"
#include "ui/UiFwd.hpp"

class QBoxLayout;

namespace chatterino::ui
{
    class SplitHeader;
    class SplitInput;

    class Split : public ab::BaseWidget
    {
        Q_OBJECT

    public:
        Split(Application& app);

    protected:
        void mousePressEvent(QMouseEvent*) override;

    private:
        void initLayout();
        void setRoom(const RoomPtr&);
        void showSelectDialog();

        RoomPtr room_;
        QWidget* roomView_{};  // delete when switching room

        std::unique_ptr<QWidget> selectDialog_;
        SplitHeader* header_;
        SplitInput* input_;
        Application& app_;
        QBoxLayout* contentLayout_;
        SelectChannel selectChannel_;
    };
}  // namespace chatterino::ui
