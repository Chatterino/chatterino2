#include "word.h"

Word::Word(LazyLoadedImage* image, Type type, const QString& copytext, const QString& tooltip)
{
    this->image = image;
    this->text = NULL;
    m_isImage = true;
    m_type = type;
    m_copyText = new QString(copytext);
    m_tooltip = new QString(tooltip);
}

Word::Word(const QString& text, Type type, const QString& copytext, const QString& tooltip)
{
    this->image = NULL;
    this->text = new QString(text);
    m_isImage = false;
    m_type = type;
    m_copyText = new QString(copytext);
    m_tooltip = new QString(tooltip);
}

Word::~Word()
{
    delete text;
    delete m_copyText;
    delete m_tooltip;
}
