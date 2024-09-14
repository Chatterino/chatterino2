#include "TestWidget.hpp"

#include <qboxlayout.h>
#include <qpushbutton.h>
#include <QStringBuilder>
namespace chatterino {

class TestWidgetImpl
{
public:
    TestWidgetImpl(QWidget *parent)
        : btn(new QPushButton("xd", parent))
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

}  // namespace chatterino
