#pragma once

#include "singletons/Fonts.hpp"
#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signalholder.hpp>

namespace AB_NAMESPACE {

class Label : public BaseWidget
{
public:
    explicit Label(QString text = QString(),
                   FontStyle style = FontStyle::UiMedium);
    explicit Label(BaseWidget *parent, QString text = QString(),
                   FontStyle style = FontStyle::UiMedium);

    const QString &getText() const;
    void setText(const QString &text);

    FontStyle getFontStyle() const;
    void setFontStyle(FontStyle style);

    bool getCentered() const;
    void setCentered(bool centered);

    bool getHasOffset() const;
    void setHasOffset(bool hasOffset);

protected:
    virtual void scaleChangedEvent(float scale_) override;
    virtual void paintEvent(QPaintEvent *) override;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

private:
    void updateSize();
    int getOffset();

    QString text_;
    FontStyle fontStyle_;
    QSize preferedSize_;
    bool centered_ = false;
    bool hasOffset_ = true;

    pajlada::Signals::SignalHolder connections_;
};

}  // namespace AB_NAMESPACE
