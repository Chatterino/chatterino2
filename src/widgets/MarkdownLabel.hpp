#pragma once

#include "widgets/Label.hpp"

#include <memory>

class QTextDocument;

namespace chatterino {

/// @brief A Label that supports rendering markdown text
///
/// MarkdownLabel inherits from Label and adds markdown rendering capabilities.
/// It automatically handles markdown document creation and rendering.
class MarkdownLabel : public Label
{
public:
    explicit MarkdownLabel(BaseWidget *parent, QString text,
                           FontStyle style = FontStyle::UiMedium);

    void setText(const QString &text);

protected:
    void paintEvent(QPaintEvent * /*event*/) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void updateSize() override;

    std::unique_ptr<QTextDocument> markdownDocument_;
};

}  // namespace chatterino
