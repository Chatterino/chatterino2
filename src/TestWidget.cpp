#include "TestWidget.hpp"

#include "controllers/highlights/HighlightPhrase.hpp"
#include "widgets/helper/color/ColorButton.hpp"

#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <QStringBuilder>
#include <qtextedit.h>
namespace chatterino {

class TestWidgetImpl
{
public:
    TestWidgetImpl(QWidget *parent)
        : btn(new QPushButton("xd5", parent))
    {
    }

    QPushButton *btn;
};

TestWidget::TestWidget(QWidget *parent)
    : QWidget(parent)
    , impl(new TestWidgetImpl(this))
{
    auto *layout = new QVBoxLayout(this);

    layout->addWidget(impl->btn);
}

void TestWidget::update(const QString &data)
{
    this->impl->btn->setText("AAAAA" % data);
}

TestWidget2::TestWidget2(QWidget *parent, const HighlightPhrase &data)
    : QWidget(parent)
{
    auto *layout = new QGridLayout(this);

    auto *lbl = new QLineEdit(data.getPattern());
    layout->addWidget(lbl, 0, 0, 1, 2);

    {
        auto *cb = new ColorButton(*data.getColor());
        layout->addWidget(cb, 0, 3);
    }

    int row = 0;
    int column = 0;

    {
        auto *cb = new QCheckBox("Show in mentions");
        cb->setChecked(data.showInMentions());
        layout->addWidget(cb, 1, 0);
    }

    {
        auto *cb = new QCheckBox("Flash taskbar");
        cb->setChecked(data.hasAlert());
        layout->addWidget(cb, 1, 1);
    }

    {
        auto *cb = new QCheckBox("Enable regex");
        cb->setChecked(data.isRegex());
        layout->addWidget(cb, 2, 0);
    }

    {
        auto *cb = new QCheckBox("Case-sensitive");
        cb->setChecked(data.isCaseSensitive());
        layout->addWidget(cb, 2, 1);
    }

    {
        auto *cb = new QCheckBox("Play sound");
        cb->setChecked(data.hasSound());
        layout->addWidget(cb, 3, 0);
    }

    {
        auto *cb = new QCheckBox("Custom sound");
        // cb->setChecked(data.hasSound());
        layout->addWidget(cb, 3, 1);
    }

    // layout->addLayout(gridLayout);

    layout->setSizeConstraint(QLayout::SetFixedSize);
}

}  // namespace chatterino
