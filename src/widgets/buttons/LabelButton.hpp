#pragma once

#include "widgets/buttons/Button.hpp"

#include <QHBoxLayout>
#include <QLabel>

namespace chatterino {

/// @brief A button with a label on top.
///
/// The content can be set with #setText(), #setPixmap(), and #setMovie().
class LabelButton : public Button
{
public:
    LabelButton(const QString &text = {}, BaseWidget *parent = nullptr,
                QSize padding = {6, 0});

    /// @brief Returns the current text
    ///
    /// If no text has been set this will return an empty string.
    [[nodiscard]] QString text() const;

    /// @brief Sets the label's text to @a text.
    ///
    /// Any previous content is cleared.
    /// Supports Qt's HTML subset.
    void setText(const QString &text);

    /// @brief Sets the label's pixmap to @a pixmap.
    ///
    /// Any previous content is cleared.
    ///
    /// Prefer using a `PixmapButton` or `SvgButton`.
    void setPixmap(const QPixmap &pixmap);

    /// @brief Sets the label contents to @a movie.
    ///
    /// Any previous content is cleared.
    /// The label does NOT take ownership of the movie.
    void setMovie(QMovie *movie);

    /// @brief Returns the padding inside the button.
    ///
    /// `width` is the padding applied horizontally (left and right).
    /// `height` is the padding applied vertically (top and bottom).
    ///
    /// By default, the button has no vertical padding and a horizontal padding of 6.
    [[nodiscard]] QSize padding() const noexcept;

    /// Setter for #padding()
    void setPadding(QSize padding);

    /// Sets the label to display rich text (Qt's HTML subset)
    void setRichText();

protected:
    void paintContent(QPainter &painter) override;

private:
    void updatePadding();

    QHBoxLayout layout_;
    QLabel label_;
    QSize padding_;
};

}  // namespace chatterino
