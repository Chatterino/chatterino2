#include "widgets/MarkdownLabel.hpp"

#include "Application.hpp"
#include "singletons/Theme.hpp"

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QPainter>
#include <QTextDocument>
#include <QUrl>

namespace chatterino {

MarkdownLabel::MarkdownLabel(BaseWidget *parent, QString text, FontStyle style)
    : Label(parent, std::move(text), style)
{
    this->initializeMarkdownDocument();
}

void MarkdownLabel::setText(const QString &text)
{
    if (this->text_ != text)
    {
        this->text_ = text;
        if (this->markdownDocument_)
        {
            this->markdownDocument_->setMarkdown(text);
        }
        this->updateSize();
        this->update();
    }
}

void MarkdownLabel::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    painter.setFont(
        getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

    // draw text
    QRectF textRect = this->textRect();

    if (this->markdownDocument_ && !this->text_.isEmpty())
    {
        QColor textColor =
            this->theme ? this->theme->messages.textColors.regular : Qt::black;

        this->markdownDocument_->setTextWidth(textRect.width());
        this->markdownDocument_->setDefaultFont(
            getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));
        this->markdownDocument_->setMarkdown(this->text_);

        QPalette docPalette = this->palette();
        docPalette.setColor(QPalette::Text, textColor);
        docPalette.setColor(QPalette::WindowText, textColor);

        painter.setPen(textColor);

        painter.save();
        painter.translate(textRect.topLeft());

        // create a rendering context using our text color and document palette
        QAbstractTextDocumentLayout::PaintContext paintContext;
        paintContext.palette = docPalette;
        paintContext.clip = QRectF(0, 0, textRect.width(), textRect.height());
        this->markdownDocument_->documentLayout()->draw(&painter, paintContext);

        painter.restore();
    }
    else
    {
        // Fall back to the base Label rendering if no markdown document
        Label::paintEvent(nullptr);
        return;
    }
}

void MarkdownLabel::mousePressEvent(QMouseEvent *event)
{
    if (this->markdownDocument_ && event->button() == Qt::LeftButton)
    {
        QRectF textRect = this->textRect();
        QPointF pos = event->pos() - textRect.topLeft();

        QString anchor =
            this->markdownDocument_->documentLayout()->anchorAt(pos);
        if (!anchor.isEmpty())
        {
            QUrl url(anchor);

            // Validate the URL and add scheme if missing
            if (!url.isValid())
            {
                return;
            }

            // If the URL doesn't have a scheme, assume it's http
            if (url.scheme().isEmpty())
            {
                url.setScheme("http");
            }

            // Only open URLs with safe schemes
            QString scheme = url.scheme().toLower();
            if (scheme == "http" || scheme == "https" || scheme == "ftp" ||
                scheme == "file" || scheme == "mailto")
            {
                QDesktopServices::openUrl(url);
            }
            return;
        }
    }

    Label::mousePressEvent(event);
}

void MarkdownLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (this->markdownDocument_)
    {
        QRectF textRect = this->textRect();
        QPointF pos = event->pos() - textRect.topLeft();

        QString anchor =
            this->markdownDocument_->documentLayout()->anchorAt(pos);
        if (!anchor.isEmpty())
        {
            this->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            this->setCursor(Qt::ArrowCursor);
        }
    }

    Label::mouseMoveEvent(event);
}

void MarkdownLabel::updateSize()
{
    this->currentPadding_ = this->basePadding_.toMarginsF() * this->scale();

    if (this->markdownDocument_ && !this->text_.isEmpty())
    {
        this->markdownDocument_->setDefaultFont(
            getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

        this->markdownDocument_->setMarkdown(this->text_);

        // Use word wrap width if enabled, otherwise use a reasonable default
        qreal testWidth = this->wordWrap_
                              ? 400.0 * this->scale()
                              : this->markdownDocument_->idealWidth();
        this->markdownDocument_->setTextWidth(testWidth);

        auto yPadding =
            this->currentPadding_.top() + this->currentPadding_.bottom();

        auto height = this->markdownDocument_->size().height() + yPadding;
        auto width = qMin(this->markdownDocument_->idealWidth(), testWidth) +
                     this->currentPadding_.left() +
                     this->currentPadding_.right();

        this->sizeHint_ = QSizeF(width, height).toSize();
        this->minimumSizeHint_ = this->sizeHint_;

        this->updateGeometry();
    }
    else
    {
        // Fall back to base Label size calculation
        Label::updateSize();
    }
}

void MarkdownLabel::initializeMarkdownDocument()
{
    this->markdownDocument_ = std::make_unique<QTextDocument>();
    if (!this->text_.isEmpty())
    {
        this->markdownDocument_->setMarkdown(this->text_);
    }
}

}  // namespace chatterino
