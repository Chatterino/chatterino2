#include "widgets/helper/DebugPopup.hpp"

#include "common/Literals.hpp"
#include "util/Clipboard.hpp"
#include "util/DebugCount.hpp"

#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace chatterino {

using namespace literals;

DebugPopup::DebugPopup()
{
    auto *layout = new QVBoxLayout(this);
    auto *text = new QLabel(this);
    auto *timer = new QTimer(this);
    auto *copyButton = new QPushButton(u"&Copy"_s);

    QObject::connect(timer, &QTimer::timeout, [text] {
        text->setText(DebugCount::getDebugText());
    });
    timer->start(300);
    text->setText(DebugCount::getDebugText());

    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    layout->addWidget(text);
    layout->addWidget(copyButton, 1);

    QObject::connect(copyButton, &QPushButton::clicked, this, [text] {
        crossPlatformCopy(text->text());
    });
}

}  // namespace chatterino
