#pragma once

#include "widgets/BaseWidget.hpp"

namespace chatterino {

class LiveIndicator : public BaseWidget
{
public:
    LiveIndicator(QWidget *parent = nullptr);

    void setViewers(int viewers);

protected:
    void scaleChangedEvent(float newScale) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateScale();

    bool hovered = false;
};

}  // namespace chatterino
