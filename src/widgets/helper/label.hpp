#pragma once

#include "singletons/fontmanager.hpp"
#include "widgets/basewidget.hpp"

namespace chatterino {
namespace widgets {

class Label : public BaseWidget
{
public:
    Label(BaseWidget *parent);

    const QString &getText() const;
    void setText(const QString &text);

    FontStyle getFontStyle() const;
    void setFontStyle(FontStyle style);

protected:
    virtual void scaleChangedEvent(float scale) override;
    virtual void paintEvent(QPaintEvent *event) override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

private:
    QSize preferedSize;
    QString text;
    FontStyle fontStyle = FontStyle::ChatMedium;
};

}  // namespace widgets
}  // namespace chatterino
