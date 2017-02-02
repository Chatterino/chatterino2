#include "widgets/chatwidgetview.h"
#include "channels.h"
#include "colorscheme.h"
#include "messages/message.h"
#include "messages/wordpart.h"
#include "settings.h"
#include "widgets/chatwidget.h"

#include <math.h>
#include <QDebug>
#include <QPainter>
#include <functional>

namespace chatterino {
namespace widgets {

ChatWidgetView::ChatWidgetView(ChatWidget *parent)
    : QWidget()
    , chatWidget(parent)
    , scrollbar(this)
{
    this->scrollbar.setSmallChange(5);

    QObject::connect(&Settings::getInstance(), &Settings::wordTypeMaskChanged,
                     this, &ChatWidgetView::wordTypeMaskChanged);

    this->scrollbar.getCurrentValueChanged().connect([this] { update(); });
}

ChatWidgetView::~ChatWidgetView()
{
    QObject::disconnect(&Settings::getInstance(),
                        &Settings::wordTypeMaskChanged, this,
                        &ChatWidgetView::wordTypeMaskChanged);
}

bool
ChatWidgetView::layoutMessages()
{
    bool showScrollbar = false;

    auto messages = chatWidget->getMessagesSnapshot();

    bool redraw = false;

    //    for (std::shared_ptr<messages::Message> &message : messages) {
    //        redraw |= message.get()->layout(this->width(), true);
    //    }

    redraw = true;

    int h = this->height() - 8;

    for (int i = messages.getLength() - 1; i >= 0; i--) {
        auto *message = messages[i].get();

        message->layout(this->width(), true);

        h -= message->getHeight();

        if (h < 0) {
            this->scrollbar.setLargeChange((messages.getLength() - i) +
                                           (qreal)h / message->getHeight());
            this->scrollbar.setDesiredValue(this->scrollbar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    this->scrollbar.setVisible(showScrollbar);

    this->scrollbar.setMaximum(messages.getLength());

    return redraw;
}

void
ChatWidgetView::resizeEvent(QResizeEvent *)
{
    this->scrollbar.resize(this->scrollbar.width(), height());
    this->scrollbar.move(width() - this->scrollbar.width(), 0);

    layoutMessages();
}

void
ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    //    auto c = this->chatWidget->getChannel();

    QColor color;

    ColorScheme &scheme = ColorScheme::getInstance();

    // code for tesing colors
    /*
    static ConcurrentMap<qreal, QImage *> imgCache;

    std::function<QImage *(qreal)> getImg = [&scheme](qreal light) {
        return imgCache.getOrAdd(light, [&scheme, &light] {
            QImage *img = new QImage(150, 50, QImage::Format_RGB32);

            QColor color;

            for (int j = 0; j < 50; j++) {
                for (qreal i = 0; i < 150; i++) {
                    color = QColor::fromHslF(i / 150.0, light, j / 50.0);

                    scheme.normalizeColor(color);

                    img->setPixelColor(i, j, color);
                }
            }

            return img;
        });
    };

    for (qreal k = 0; k < 4.8; k++) {
        auto img = getImg(k / 5);

        painter.drawImage(QRect(k * 150, 0, 150, 150), *img);
    }

    painter.fillRect(QRect(0, 9, 500, 2), QColor(0, 0, 0));*/

    //    if (c == NULL)
    //        return;

    //    auto messages = c->getMessageSnapshot();

    auto messages = chatWidget->getMessagesSnapshot();

    int start = this->scrollbar.getCurrentValue();

    if (start >= messages.getLength()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() *
              (fmod(this->scrollbar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getLength(); ++i) {
        messages::MessageRef *messageRef = messages[i].get();

        for (messages::WordPart const &wordPart : messageRef->getWordParts()) {
            painter.setPen(QColor(255, 0, 0));
            painter.drawRect(wordPart.getX(), wordPart.getY() + y,
                             wordPart.getWidth(), wordPart.getHeight());

            // image
            if (wordPart.getWord().isImage()) {
                messages::LazyLoadedImage &lli = wordPart.getWord().getImage();

                const QPixmap *image = lli.getPixmap();

                if (image != NULL) {
                    painter.drawPixmap(
                        QRect(wordPart.getX(), wordPart.getY() + y,
                              wordPart.getWidth(), wordPart.getHeight()),
                        *image);
                }
            }
            // text
            else {
                QColor color = wordPart.getWord().getColor();

                ColorScheme::getInstance().normalizeColor(color);

                painter.setPen(color);
                painter.setFont(wordPart.getWord().getFont());

                painter.drawText(
                    QRectF(wordPart.getX(), wordPart.getY() + y, 10000, 10000),
                    wordPart.getText(),
                    QTextOption(Qt::AlignLeft | Qt::AlignTop));
            }
        }

        y += messageRef->getHeight();

        if (y > height()) {
            break;
        }
    }
}

void
ChatWidgetView::wheelEvent(QWheelEvent *event)
{
    this->scrollbar.setDesiredValue(
        this->scrollbar.getDesiredValue() -
            event->delta() / 10.0 *
                Settings::getInstance().mouseScrollMultiplier.get(),
        true);
}
}
}
