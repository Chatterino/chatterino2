#include "DebugPopup.hpp"

#include "util/DebugCount.hpp"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

namespace chatterino {

DebugPopup::DebugPopup()
{
    auto *layout = new QHBoxLayout(this);
    auto *text = new QLabel(this);
    auto *timer = new QTimer(this);

    timer->setInterval(300);
    QObject::connect(timer, &QTimer::timeout, [text] {
        text->setText(DebugCount::getDebugText());
    });
    timer->start();

    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    layout->addWidget(text);
}

}  // namespace chatterino
