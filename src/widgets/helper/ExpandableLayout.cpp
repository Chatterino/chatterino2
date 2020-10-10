#include "widgets/helper/ExpandableLayout.hpp"

#include "widgets/Label.hpp"

namespace chatterino {

ExpandableLayout::ExpandableLayout(const QString &title, QWidget *parent)
    : BaseWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    this->toggleButton_ = new QToolButton(this);
    this->content_ = new QWidget(this);

    this->toggleButton_->setCheckable(true);
    this->toggleButton_->setText(title);
    this->toggleButton_->setArrowType(Qt::ArrowType::DownArrow);
    this->toggleButton_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    this->toggleButton_->setStyleSheet("QToolButton {border: none;}");

    QObject::connect(this->toggleButton_, &QToolButton::toggled, this,
                     &ExpandableLayout::toggle);
    // Forward QToolButton::toggled signal
    QObject::connect(this->toggleButton_, &QToolButton::toggled,
                     [this](bool toggled) { emit buttonToggled(toggled); });

    layout->addWidget(this->toggleButton_);
    layout->setAlignment(this->toggleButton_, Qt::AlignHCenter);

    layout->addWidget(this->content_);

    this->setLayout(layout);

    // Collapsed per default
    this->toggle(false);
}

void ExpandableLayout::toggle(bool expand)
{
    this->toggleButton_->setArrowType(expand ? Qt::ArrowType::UpArrow
                                             : Qt::ArrowType::DownArrow);
    this->content_->setVisible(expand);
}

void ExpandableLayout::setContent(QLayout *layout)
{
    if (auto *prevLayout = this->content_->layout(); prevLayout)
    {
        /*
         * An existing layout on a QWidget must be deleted before a new one can
         * be assigned (cf. https://doc.qt.io/qt-5/layout.html). QLayout does
         * not inherit from QObject so we cannot use QObject::deleteLater.
         */
        delete prevLayout;
    }

    this->content_->setLayout(layout);
}

}  // namespace chatterino
