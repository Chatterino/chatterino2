// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWidget.hpp"

#include <chrono>

class QHBoxLayout;
class QLabel;
class QTimer;
class QVBoxLayout;

namespace chatterino {

/// Base for banners shown between the split header and the chat view.
/// Subclasses must call scaleChangedEvent() at the end of their constructor,
/// once their own widgets exist.
class SplitBanner : public BaseWidget
{
    Q_OBJECT

public:
    explicit SplitBanner(QWidget *parent = nullptr);

protected:
    static constexpr auto MUTED_STYLE = "color: #adadb8;";

    QLabel *headerLabel() const;
    /// Widgets added here sit after the leading label and a stretch.
    QHBoxLayout *headerRow() const;
    /// Content added here sits below the header row.
    QVBoxLayout *contentBox() const;
    /// Not placed in a layout - subclasses add it to the row they want.
    QLabel *countdownLabel() const;

    /// Ticks once immediately, then every second.
    void startCountdown();
    void stopCountdown();

    void startAutoHide(std::chrono::milliseconds delay);
    void stopAutoHide();

    virtual void tickCountdown();
    virtual void autoHide();

    /// `H:MM:SS` from an hour up, `M:SS` below it. Rounds up.
    static QString formatCountdown(qint64 millis);

    /// Scales the header and countdown labels. Overrides have to call this.
    void scaleChangedEvent(float newScale) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *headerLabel_ = nullptr;
    QLabel *countdownLabel_ = nullptr;
    QHBoxLayout *headerRow_ = nullptr;
    QVBoxLayout *contentBox_ = nullptr;

    QTimer *countdownTimer_ = nullptr;
    QTimer *autoHideTimer_ = nullptr;
};

}  // namespace chatterino
