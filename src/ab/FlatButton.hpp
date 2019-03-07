#pragma once

#include "ab/BaseWidget.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>
#include <optional>

namespace ab
{
    class FlatButton : public BaseWidget
    {
        Q_OBJECT

        struct ClickEffect
        {
            double progress = 0.0;
            QPoint position;

            ClickEffect(QPoint _position)
                : position(_position)
            {
            }
        };

    public:
        FlatButton(BaseWidget* parent = nullptr);

        void setPixmap(const QPixmap& pixmap_);
        const QPixmap& getPixmap() const;

        void setDim(bool value);
        bool getDim() const;
        qreal getCurrentDimAmount() const;

        void setEnable(bool value);
        bool getEnable() const;

        void setEnableMargin(bool value);
        bool getEnableMargin() const;

        void setRipple(bool);
        bool ripple();
        Q_PROPERTY(bool ripple READ ripple WRITE setRipple)

        void setHover(bool);
        bool hover();
        Q_PROPERTY(bool hover READ hover WRITE setHover)

        void setMenu(std::unique_ptr<QMenu> menu);

        void setChild(QWidget* widget);

    signals:
        void leftClicked();
        void clicked(Qt::MouseButton button);
        void leftMousePress();

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void enterEvent(QEvent*) override;
        virtual void leaveEvent(QEvent*) override;
        virtual void mousePressEvent(QMouseEvent* event) override;
        virtual void mouseReleaseEvent(QMouseEvent* event) override;
        virtual void mouseMoveEvent(QMouseEvent* event) override;

        void paint(QPainter& painter);
        void fancyPaint(QPainter& painter);

        bool enabled_{true};
        bool selected_{false};
        bool mouseOver_{false};
        bool mouseDown_{false};
        bool menuVisible_{false};

    private:
        void onMouseEffectTimeout();
        void showMenu();

        QPixmap pixmap_{};
        QPixmap pixmapScaled_{};
        bool dimPixmap_{true};
        bool enableMargin_{true};
        bool ripple_{true};
        bool hover_{true};
        QPoint mousePos_{};
        double hoverMultiplier_{0.0};
        QTimer effectTimer_{};
        std::vector<ClickEffect> clickEffects_{};
        std::optional<QColor> mouseEffectColor_{};
        std::unique_ptr<QMenu> menu_{};
    };
}  // namespace ab
