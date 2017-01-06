#include "scrollbarhighlight.h"
#include "colorscheme.h"

ScrollBarHighlight::ScrollBarHighlight(float position, int colorIndex, Style style, QString tag)
    : m_position(position),
      m_colorIndex(std::max(0, std::min(ColorScheme::instance().HighlightColorCount, colorIndex))),
      m_style(style),
      m_tag(tag),
      next(NULL)
{

}
