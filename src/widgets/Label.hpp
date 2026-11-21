// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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

    void setPadding(QMargins padding);

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

    virtual void updateSize();
    QRectF textRect() const;

    /// Returns the current font style's font metric based on the current scale.
    QFontMetricsF getFontMetrics() const;

    /// Calculate the new elided text based on text_
    ///
    /// Return true if the elided text changed
    bool updateElidedText(const QFontMetricsF &fontMetrics, qreal width);

    QString text_;
    FontStyle fontStyle_;
    QSize sizeHint_;
    QSize minimumSizeHint_;

    /// The user specified padding (scale agnostic)
    QMargins basePadding_;
    /// The actual scaled padding
    QMarginsF currentPadding_;

    bool centered_ = false;
    bool wordWrap_ = false;
    bool shouldElide_ = false;
    /// The text, but elided. Only set if shouldElide_ is true
    QString elidedText_;

    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
