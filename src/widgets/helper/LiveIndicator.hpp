#pragma once

#include "widgets/BaseWidget.hpp"

#include <optional>

namespace chatterino {

class LiveIndicator : public BaseWidget
{
public:
    LiveIndicator(int paddingRight, QWidget *parent = nullptr);

    void setViewers(std::optional<int> viewers);

protected:
    void scaleChangedEvent(float newScale) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateScale();

    std::optional<int> viewers;
    bool hovered = false;
    int paddingRight;
};

}  // namespace chatterino
