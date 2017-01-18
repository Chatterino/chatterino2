#include "messages/word.h"

namespace chatterino {
namespace messages {

// Image word
Word::Word(LazyLoadedImage *image, Type type, const QString &copytext,
           const QString &tooltip, const Link &link)
    : image(image)
    , text()
    , color()
    , _isImage(true)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
    , link(link)
    , characterWidthCache()
{
    image->getWidth();  // professional segfault test
}

// Text word
Word::Word(const QString &text, Type type, const QColor &color,
           const QString &copytext, const QString &tooltip, const Link &link)
    : image(NULL)
    , text(text)
    , color(color)
    , _isImage(false)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
    , link(link)
    , characterWidthCache()
{
}
}
}
