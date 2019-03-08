#pragma once

#include <boost/optional.hpp>

#include "ab/BaseWidget.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>

namespace chatterino
{
    class Button : public ab::BaseWidget
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
        Button(BaseWidget* parent = nullptr);

        void setMouseEffectColor(boost::optional<QColor> color);
        void setPixmap(const QPixmap& pixmap_);
        const QPixmap& getPixmap() const;

        void setDim(bool value);
        bool getDim() const;
        qreal getCurrentDimAmount() const;

        void setEnable(bool value);
        bool getEnable() const;

        void setEnableMargin(bool value);
        bool getEnableMargin() const;

        void setBorderColor(const QColor& color);
        const QColor& getBorderColor() const;

        void setMenu(std::unique_ptr<QMenu> menu);

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

        void fancyPaint(QPainter& painter);

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
        bool dimPixmap_{true};
        bool enableMargin_{true};
        QPoint mousePos_{};
        double hoverMultiplier_{0.0};
        QTimer effectTimer_{};
        std::vector<ClickEffect> clickEffects_{};
        boost::optional<QColor> mouseEffectColor_{};
        std::unique_ptr<QMenu> menu_{};
    };

}  // namespace chatterino
