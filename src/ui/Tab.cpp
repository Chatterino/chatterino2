#include "Tab.hpp"

#include <QLabel>

#include "ab/Row.hpp"

namespace chatterino::ui
{
    // TABBUTTON
    void TabButton::setType(TabButtonType value)
    {
        this->type_ = value;
        this->update();
    }

    TabButtonType TabButton::type() const
    {
        return this->type_;
    }

    void TabButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        this->paint(painter);

        auto rect = this->contentsRect();

        // TODO: change rect size if scaled

        switch (this->type_)
        {
            case TabButtonType::Close:
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.setBrush(this->palette().foreground());
                painter.drawLine(rect.topLeft(), rect.bottomRight());
                painter.drawLine(rect.bottomLeft(), rect.topRight());
                break;

            case TabButtonType::Live:
                painter.setBrush(QColor("#f00"));
                painter.drawEllipse(rect);
                break;

            default:;
        }
    }

    // TAB
    Tab::Tab()
    {
        this->row().addWidget(this->button_ = new TabButton);

        QObject::connect(
            this->button_, &TabButton::clicked, this, [this](auto button) {
                if (button == Qt::LeftButton)
                    this->close();
            });
    }

    Tab::Tab(const QString& text)
        : Tab()
    {
        this->label().setText(text);
    }

    void Tab::setShowX(bool value)
    {
        this->showX_ = value;

        this->button_->setType(
            value ? TabButtonType::Close : TabButtonType::None);
    }

    bool Tab::showX() const
    {
        return this->showX_;
    }
}  // namespace chatterino::ui
