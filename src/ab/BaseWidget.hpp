#pragma once

#include <QStyleOption>
#include <QWidget>
//#include <boost/optional.hpp>

namespace ab
{
    class Theme;
    class BaseWindow;

    void setPropertyAndPolish(
        QWidget* widget, const char* name, const QVariant& value);
    void polishChildren(QWidget* widget);
    QPoint middle(const QPoint& a, const QPoint& b);

    class BaseWidget : public QFrame
    {
        Q_OBJECT

    public:
        explicit BaseWidget(
            QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

        virtual float scale() const;

        // TODO: move to BaseWindow
        //    boost::optional<float> overrideScale() const;
        //    void setOverrideScale(boost::optional<float>);

        QSize scaleIndependantSize() const;
        int scaleIndependantWidth() const;
        int scaleIndependantHeight() const;
        void setScaleIndependantSize(int width, int height);
        void setScaleIndependantSize(QSize);
        void setScaleIndependantWidth(int value);
        void setScaleIndependantHeight(int value);

        float qtFontScale() const;

    protected:
        void childEvent(QChildEvent*) override;
        void showEvent(QShowEvent*) override;

        void paintEvent(QPaintEvent*) override;

        virtual void scaleChangedEvent(float scale);   // TODO: remove
        virtual void themeChangedEvent(Theme& theme);  // TODO: remove

    private:
        // float scale_{1.f};
        // boost::optional<float> overrideScale_;
        QSize scaleIndependantSize_;

        std::vector<BaseWidget*> widgets_;

        Theme* theme_{};

        friend class BaseWindow;
    };
}  // namespace ab
