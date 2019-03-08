#pragma once

#include "ab/BaseWidget.hpp"

#include <QFrame>
#include <functional>

class QHBoxLayout;
class QLabel;

namespace ab
{
    class Row;
    class Column;
    class FlatButton;
    class EffectLabel;
    class TitleBarButton;
    enum class TitleBarButtonStyle;

    class WindowContent : public QFrame
    {
        Q_OBJECT
    };

    class BaseWindow : public BaseWidget
    {
        Q_OBJECT

    public:
        enum Flags {
            None = 0,
            EnableCustomFrame = 1,
            Frameless = 2,
            TopMost = 4,
            DisableCustomScaling = 8,
            FramelessDraggable = 16,
        };

        enum ActionOnFocusLoss { Nothing, Delete, Close, Hide };

        explicit BaseWindow(Flags flags_ = EnableCustomFrame);

        bool hasCustomWindowFrame();
        void addTitleBarButton(QWidget* widget);

        //    TitleBarButton* addTitleBarButton(const TitleBarButtonStyle&
        //    style,
        //                                      std::function<void()>
        //                                      onClicked);
        //    EffectLabel* addTitleBarLabel(std::function<void()> onClicked);

        void setStayInScreenRect(bool value);
        bool getStayInScreenRect() const;

        void setActionOnFocusLoss(ActionOnFocusLoss value);
        ActionOnFocusLoss getActionOnFocusLoss() const;

        void moveTo(QWidget* widget, QPoint point, bool offset = true);

        virtual float scale() const override;

        Flags getFlags();

        void setCenterWidget(QWidget* widget);
        void setCenterLayout(QLayout* layout);
        /// use setCenterWidget/setCenterLayout instead
        void setLayout(QLayout*);
        const QString& scalableQss() const;
        void setScalableQss(const QString&);

    signals:
        void scaleChanged();
        void closing();

    protected:
        virtual bool nativeEvent(
            const QByteArray& eventType, void* message, long* result) override;

        virtual void changeEvent(QEvent*) override;
        virtual void leaveEvent(QEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void moveEvent(QMoveEvent*) override;
        virtual void closeEvent(QCloseEvent*) override;

        virtual bool event(QEvent* event) override;
        virtual void wheelEvent(QWheelEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        QPointF movingRelativePos;
        bool moving{};

        void updateScalableQss();
        void hideMaximize();

    private:
        void initLayout();
        void initCustomWindowFrame();
        ab::Row* makeCustomTitleBar();
        void moveIntoDesktopRect(QWidget* parent);
        void onFocusLost();
        void removeBackground();

        bool handleDPICHANGED(MSG* msg);
        bool handleSHOWWINDOW(MSG* msg);
        bool handleNCCALCSIZE(MSG* msg, long* result);
        bool handleSIZE(MSG* msg);
        bool handleNCHITTEST(MSG* msg, long* result);

        bool enableCustomFrame_;
        ActionOnFocusLoss actionOnFocusLoss_ = Nothing;
        bool frameless_;
        bool stayInScreenRect_ = false;
        bool shown_ = false;
        Flags flags_;
        float nativeScale_ = 1;
        QString scalableQss_;

        struct
        {
            ab::Row* titlebar{};
            Column* windowLayout{};
            QLabel* titleLabel{};
            Column* layoutBase{};
            TitleBarButton* maxButton{};

            std::vector<QWidget*> buttons;
        } ui_;

        friend class BaseWidget;
    };

    class Dialog : public BaseWindow
    {
        Q_OBJECT

    public:
        Dialog();
    };
}  // namespace ab
