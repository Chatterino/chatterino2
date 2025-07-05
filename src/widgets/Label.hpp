#pragma once

#include "singletons/Fonts.hpp"
#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signalholder.hpp>

class QFontMetricsF;

namespace chatterino {

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

    /// Enable or disable horizontal padding
    void setHasPadding(bool hasPadding);

    bool getWordWrap() const;
    void setWordWrap(bool wrap);

    /// Sets whether the text should elide if there's not enough room to
    /// render the current text.
    void setShouldElide(bool shouldElide);

protected:
    void scaleChangedEvent(float scale_) override;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void updateSize();

    /// Returns the horizontal padding, or 0 if hasPadding_ is false
    int getPadding() const;

    /// Returns the current font style's font metric based on the current scale.
    QFontMetricsF getFontMetrics() const;

    /// Returns the width of this content without padding
    qreal getInnerWidth() const;

    /// Calculate the new elided text based on text_
    ///
    /// Return true if the elided text changed
    bool updateElidedText(const QFontMetricsF &fontMetrics, qreal width);

    QString text_;
    FontStyle fontStyle_;
    QSize sizeHint_;
    QSize minimumSizeHint_;
    bool centered_ = false;
    bool hasPadding_ = true;
    bool wordWrap_ = false;
    bool shouldElide_ = false;
    /// The text, but elided. Only set if shouldElide_ is true
    QString elidedText_;

    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
