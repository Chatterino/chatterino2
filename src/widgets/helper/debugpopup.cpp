#include "debugpopup.hpp"

#include "util/debugcount.hpp"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

namespace chatterino {
namespace widgets {

DebugPopup::DebugPopup()
{
    auto *layout = new QHBoxLayout(this);
    auto *text = new QLabel(this);
    auto *timer = new QTimer(this);

    timer->setInterval(1000);
    QObject::connect(timer, &QTimer::timeout,
                     [text] { text->setText(util::DebugCount::getDebugText()); });
    timer->start();

    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    layout->addWidget(text);
}

}  // namespace widgets
}  // namespace chatterino
