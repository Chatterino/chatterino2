#include "widgets/buttons/SvgButton.hpp"

#include "singletons/Theme.hpp"

#include <QSvgRenderer>

namespace chatterino {

SvgButton::SvgButton(Src source, BaseWidget *parent, QSize padding)
    : Button(parent)
    , source_(std::move(source))
    , svg_(new QSvgRenderer(this->currentSvgPath(), this))
    , padding_(padding)
{
    this->svg_->setAspectRatioMode(Qt::KeepAspectRatio);
    this->setContentCacheEnabled(true);
}

void SvgButton::setSource(Src source)
{
    this->source_ = std::move(source);
    this->svg_->load(this->currentSvgPath());
    this->invalidateContent();
}

void SvgButton::setPadding(QSize padding)
{
    if (this->padding_ == padding)
    {
        return;
    }

    this->padding_ = padding;
    this->invalidateContent();
}

void SvgButton::themeChangedEvent()
{
    Button::themeChangedEvent();

    if (this->source_.dark == this->source_.light)
    {
        return;
    }
    this->svg_->load(this->currentSvgPath());
    this->invalidateContent();
}

void SvgButton::scaleChangedEvent(float scale)
{
    Button::scaleChangedEvent(scale);

    this->invalidateContent();
}

void SvgButton::resizeEvent(QResizeEvent *e)
{
    Button::resizeEvent(e);

    this->invalidateContent();
}

void SvgButton::paintContent(QPainter &painter)
{
    QSize actualPadding = this->scale() * this->padding_;
    QPoint topLeft{actualPadding.width(), actualPadding.height()};
    QSize contentSize = this->size() - 2 * actualPadding;
    this->svg_->render(&painter, {topLeft, contentSize});
}

QString SvgButton::currentSvgPath() const
{
    if (this->theme->isLightTheme())
    {
        return this->source_.light;
    }
    return this->source_.dark;
}

}  // namespace chatterino
