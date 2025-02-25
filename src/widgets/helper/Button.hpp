#pragma once

#include "widgets/BaseWidget.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include <optional>

namespace chatterino {

class Button : public BaseWidget
{
    Q_OBJECT

    struct ClickEffect {
        double progress = 0.0;
        QPoint position;

        ClickEffect(QPoint _position)
            : position(_position)
        {
        }
    };

public:
    enum class Dim { None, Some, Lots };

    Button(BaseWidget *parent = nullptr);

    void setMouseEffectColor(std::optional<QColor> color);
    void setPixmap(const QPixmap &pixmap_);
    const QPixmap &getPixmap() const;

    void setDim(Dim value);
    Dim getDim() const;
    qreal getCurrentDimAmount() const;

    void setEnable(bool value);
    bool getEnable() const;

    void setEnableMargin(bool value);
    bool getEnableMargin() const;

    void setBorderColor(const QColor &color);
    const QColor &getBorderColor() const;

    void setMenu(std::unique_ptr<QMenu> menu);

Q_SIGNALS:
    void leftClicked();
    void clicked(Qt::MouseButton button);
    void leftMousePress();

protected:
    void paintEvent(QPaintEvent * /*event*/) override;

    /// Paint this button.
    /// This is intended for child classes that may want to paint the overlay.
    /// This function should be used after rendering the custom button,
    /// because the painter's state will be modified by this function.
    void paintButton(QPainter &painter);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * /*event*/) override;
#else
    void enterEvent(QEvent * /*event*/) override;
#endif
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void fancyPaint(QPainter &painter);

    bool enabled_{true};
    bool selected_{false};
    bool mouseOver_{false};
    bool mouseDown_{false};
    bool menuVisible_{false};

private:
    void onMouseEffectTimeout();
    void showMenu();

    QColor borderColor_{};
    QPixmap pixmap_{};
    QPixmap resizedPixmap_{};
    Dim dimPixmap_{Dim::Some};
    bool enableMargin_{true};
    QPoint mousePos_{};
    double hoverMultiplier_{0.0};
    QTimer effectTimer_{};
    std::vector<ClickEffect> clickEffects_{};
    std::optional<QColor> mouseEffectColor_{};
    std::unique_ptr<QMenu> menu_{};
};

}  // namespace chatterino
