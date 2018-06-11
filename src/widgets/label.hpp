#pragma once

#include "singletons/fontmanager.hpp"
#include "widgets/basewidget.hpp"

namespace chatterino {
namespace widgets {

class Label : public BaseWidget
{
public:
    explicit Label(QString text = QString(), FontStyle style = FontStyle::UiMedium);
    explicit Label(BaseWidget *parent, QString text = QString(),
                   FontStyle style = FontStyle::UiMedium);

    const QString &getText() const;
    void setText(const QString &text);

    FontStyle getFontStyle() const;
    void setFontStyle(FontStyle style);

    bool getCentered() const;
    void setCentered(bool centered);

    bool getHasOffset() const;
    void setHasOffset(bool centered);

protected:
    virtual void scaleChangedEvent(float scale) override;
    virtual void paintEvent(QPaintEvent *) override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

private:
    QString text_;
    FontStyle fontStyle_;
    QSize preferedSize_;
    bool centered_ = false;
    bool hasOffset_ = true;

    void updateSize();
};

}  // namespace widgets
}  // namespace chatterino
