#include "widgets/helper/SvgWidget.hpp"

#include <QSvgRenderer>

namespace chatterino {

SvgWidget::SvgWidget(QWidget *parent)
    : QWidget(parent, {})
    , renderer_(new QSvgRenderer(this))
{
    this->renderer_->setAnimationEnabled(false);
}

QSvgRenderer *SvgWidget::renderer()
{
    return this->renderer_;
}

QSize SvgWidget::sizeHint() const
{
    if (this->renderer_->isValid())
    {
        return this->renderer_->defaultSize();
    }

    return {128, 64};
}

void SvgWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter p(this);
    this->renderer_->render(&p);
}

void SvgWidget::load(const QString &file)
{
    this->renderer_->load(file);
    this->renderer_->setAnimationEnabled(false);
}

void SvgWidget::load(const QByteArray &contents)
{
    this->renderer_->load(contents);
    this->renderer_->setAnimationEnabled(false);
}

}  // namespace chatterino
