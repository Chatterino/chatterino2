#include "word.h"

Word::Word(LazyLoadedImage* image, Type type, const QString& copytext, const QString& tooltip)
{
    this->m_image = image;
    m_isImage = true;
    m_type = type;
    m_copyText = copytext;
    m_tooltip = tooltip;
}

Word::Word(const QString& text, Type type, const QString& copytext, const QString& tooltip)
{
    this->m_image = NULL;
    this->m_text = text;
    m_isImage = false;
    m_type = type;
    m_copyText = copytext;
    m_tooltip = tooltip;
}

Word::~Word()
{
}
