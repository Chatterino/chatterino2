#include "widgets/Label.hpp"

#include "Application.hpp"

#include <QPainter>
#include <QTextDocument>

namespace chatterino {

Label::Label(QString text, FontStyle style)
    : Label(nullptr, std::move(text), style)
{
}

Label::Label(BaseWidget *parent, QString text, FontStyle style)
    : BaseWidget(parent)
    , text_(std::move(text))
    , fontStyle_(style)
    , basePadding_(8, 0, 8, 0)
{
    this->connections_.managedConnect(getApp()->getFonts()->fontChanged,
                                      [this] {
                                          this->updateSize();
                                      });
    this->updateSize();
}

const QString &Label::getText() const
{
    return this->text_;
}

void Label::setText(const QString &text)
{
    if (this->text_ != text)
    {
        this->text_ = text;
        if (this->shouldElide_)
        {
            this->updateElidedText(this->getFontMetrics(),
                                   this->textRect().width());
        }
        if (this->markdownEnabled_ && this->markdownDocument_)
        {
            this->markdownDocument_->setMarkdown(text);
        }
        this->updateSize();
        this->update();
    }
}

FontStyle Label::getFontStyle() const
{
    return this->fontStyle_;
}

bool Label::getCentered() const
{
    return this->centered_;
}

void Label::setCentered(bool centered)
{
    this->centered_ = centered;
    this->updateSize();
}

void Label::setPadding(QMargins padding)
{
    this->basePadding_ = padding;
    this->updateSize();
}

bool Label::getWordWrap() const
{
    return this->wordWrap_;
}

void Label::setWordWrap(bool wrap)
{
    this->wordWrap_ = wrap;
    this->update();
}

void Label::setShouldElide(bool shouldElide)
{
    this->shouldElide_ = shouldElide;
    this->updateSize();
    this->update();
}

bool Label::getMarkdownEnabled() const
{
    return this->markdownEnabled_;
}

void Label::setMarkdownEnabled(bool enabled)
{
    this->markdownEnabled_ = enabled;
    if (enabled)
    {
        if (!this->markdownDocument_)
        {
            this->markdownDocument_ = std::make_unique<QTextDocument>();
        }
        if (!this->text_.isEmpty())
        {
            this->markdownDocument_->setMarkdown(this->text_);
        }
    }
    else
    {
        this->markdownDocument_.reset();
    }
    this->updateSize();
    this->update();
}

void Label::setFontStyle(FontStyle style)
{
    this->fontStyle_ = style;
    this->updateSize();
}

void Label::scaleChangedEvent(float /*scale*/)
{
    this->updateSize();
}

QSize Label::sizeHint() const
{
    return this->sizeHint_;
}

QSize Label::minimumSizeHint() const
{
    return this->minimumSizeHint_;
}

void Label::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    auto metrics = this->getFontMetrics();

    painter.setFont(
        getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

    // draw text
    QRectF textRect = this->textRect();

    if (this->markdownEnabled_ && this->markdownDocument_ &&
        !this->text_.isEmpty())
    {
        painter.setBrush(this->palette().windowText());

        this->markdownDocument_->setTextWidth(textRect.width());
        this->markdownDocument_->setDefaultFont(
            getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

        QString colorName = this->palette().windowText().color().name();
        this->markdownDocument_->setDefaultStyleSheet(
            QString("body { color: %1; } p { margin: 0; } h1, h2, h3, h4, h5, "
                    "h6 { margin: 0; }")
                .arg(colorName));

        this->markdownDocument_->setMarkdown(this->text_);

        painter.save();
        painter.translate(textRect.topLeft());

        this->markdownDocument_->drawContents(
            &painter, QRectF(0, 0, textRect.width(), textRect.height()));

        painter.restore();
    }
    else
    {
        auto text = [this] {
            if (this->shouldElide_)
            {
                return this->elidedText_;
            }

            return this->text_;
        }();

        qreal width = metrics.horizontalAdvance(text);
        Qt::Alignment alignment = !this->centered_ || width > textRect.width()
                                      ? Qt::AlignLeft | Qt::AlignVCenter
                                      : Qt::AlignCenter;

        painter.setBrush(this->palette().windowText());

        QTextOption option(alignment);
        if (this->wordWrap_)
        {
            option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        }
        else
        {
            option.setWrapMode(QTextOption::NoWrap);
        }
        painter.drawText(textRect, text, option);
    }

#if 0
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
#endif
}

void Label::resizeEvent(QResizeEvent *event)
{
    if (this->shouldElide_)
    {
        auto metrics = this->getFontMetrics();
        if (this->updateElidedText(metrics, this->textRect().width()))
        {
            this->update();
        }
    }

    BaseWidget::resizeEvent(event);
}

QFontMetricsF Label::getFontMetrics() const
{
    return getApp()->getFonts()->getFontMetrics(this->fontStyle_,
                                                this->scale());
}

void Label::updateSize()
{
    this->currentPadding_ = this->basePadding_.toMarginsF() * this->scale();

    auto metrics = this->getFontMetrics();

    auto yPadding =
        this->currentPadding_.top() + this->currentPadding_.bottom();

    if (this->markdownEnabled_ && this->markdownDocument_ &&
        !this->text_.isEmpty())
    {
        this->markdownDocument_->setDefaultFont(
            getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

        this->markdownDocument_->setMarkdown(this->text_);

        // Use word wrap width if enabled, otherwise use a reasonable default
        qreal testWidth = this->wordWrap_
                              ? 400.0 * this->scale()
                              : this->markdownDocument_->idealWidth();
        this->markdownDocument_->setTextWidth(testWidth);

        auto height = this->markdownDocument_->size().height() + yPadding;
        auto width = qMin(this->markdownDocument_->idealWidth(), testWidth) +
                     this->currentPadding_.left() +
                     this->currentPadding_.right();

        this->sizeHint_ = QSizeF(width, height).toSize();
        this->minimumSizeHint_ = this->sizeHint_;
    }
    else
    {
        auto height = metrics.height() + yPadding;
        if (this->shouldElide_)
        {
            this->updateElidedText(metrics, this->textRect().width());
            this->sizeHint_ = QSizeF(-1, height).toSize();
            this->minimumSizeHint_ = this->sizeHint_;
        }
        else
        {
            auto width = metrics.horizontalAdvance(this->text_) +
                         this->currentPadding_.left() +
                         this->currentPadding_.right();
            this->sizeHint_ = QSizeF(width, height).toSize();
            this->minimumSizeHint_ = this->sizeHint_;
        }
    }

    this->updateGeometry();
}

bool Label::updateElidedText(const QFontMetricsF &fontMetrics, qreal width)
{
    assert(this->shouldElide_ == true);
    auto elidedText = fontMetrics.elidedText(
        this->text_, Qt::TextElideMode::ElideRight, width);

    if (elidedText != this->elidedText_)
    {
        this->elidedText_ = elidedText;
        return true;
    }

    return false;
}

QRectF Label::textRect() const
{
    return this->rect().toRectF().marginsRemoved(this->currentPadding_);
}

}  // namespace chatterino
