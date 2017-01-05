#include "word.h"

// Image word
Word::Word(LazyLoadedImage* image, Type type, const QString& copytext, const QString& tooltip)
    : m_image(image)
    , m_text()
    , m_isImage(true)
    , m_type(type)
    , m_copyText(copytext)
    , m_tooltip(tooltip)
{
}

// Text word
Word::Word(const QString& text, Type type, const QString& copytext, const QString& tooltip)
    : m_image(nullptr)
    , m_text(text)
    , m_isImage(true)
    , m_type(type)
    , m_copyText(copytext)
    , m_tooltip(tooltip)
{
}

Word::~Word()
{
}
