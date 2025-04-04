#pragma once

#include "widgets/BaseWidget.hpp"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include <optional>

namespace chatterino {

/// @brief A generic button with click and hover effects.
///
/// This button doesn't display anything - subclasses add content to the button.
///
/// To add content in a derived class, implement #paintContent(). This gets
/// called before the click and hover effects are drawn.+
///
/// Subclasses can enable caching of the content drawn in #paintContent(), as
/// the button often repaints due to mouse movement (and the hover effects).
/// When updating the button, subclasses can invalidate the cache with
/// #invalidateContent(). Alternatively, a subclass opt out of this by calling
/// #setContentCacheEnabled().
class Button : public BaseWidget
{
    Q_OBJECT

    struct ClickEffect {
        double progress = 0.0;
        QPoint position;

        ClickEffect(QPoint _position)
            : position(_position)
        {
        }
    };

public:
    Button(BaseWidget *parent = nullptr);

    /// @brief Returns true if the button is enabled
    ///
    /// An enabled button will emit interaction events and show mouse effects.
    /// When disabled, only the content and border is shown.
    ///
    /// By default, a button is enabled.
    [[nodiscard]] bool enabled() const noexcept;

    /// Setter for #enabled()
    void setEnabled(bool enabled);

    /// Returns true if the user is hovering over the button
    [[nodiscard]] bool mouseOver() const noexcept;

    /// Returns true if the left mouse button is held down
    [[nodiscard]] bool mouseDown() const noexcept;

    /// @brief Returns true if the menu is visible
    ///
    /// @sa #menu(), #setMenu()
    [[nodiscard]] bool menuVisible() const noexcept;

    /// @brief Returns the current border color
    ///
    /// If no color is set, this will return an invalid color.
    ///
    /// The border is shown with a width of 1 above all other content if the
    /// border color is valid.
    ///
    /// By default, no border color is set.
    [[nodiscard]] QColor borderColor() const noexcept;

    /// Setter for #borderColor()
    void setBorderColor(const QColor &color);

    /// @brief Returns the current mouse effect color (if set)
    ///
    /// The mouse effect color is used for hover and click effects.
    ///
    /// By default, the color is based on the selected theme.
    [[nodiscard]] std::optional<QColor> mouseEffectColor() const;

    /// Setter for #mouseEffectColor()
    void setMouseEffectColor(std::optional<QColor> color);

    /// @brief Returns the menu associated with this button.
    ///
    /// The menu is shown when pressing the left mouse button.
    ///
    /// The return value is non-owned.
    /// If no menu is associated, `nullptr` is returned.
    ///
    /// @sa #menuVisible(), #leftMousePress()
    [[nodiscard]] QMenu *menu() const;

    /// Setter for #menu()
    void setMenu(std::unique_ptr<QMenu> menu);

Q_SIGNALS:
    /// @brief Emitted after the user left-clicked the button.
    ///
    /// A click is only emitted if the user released the mouse above the button
    /// and the button is enabled.
    void leftClicked();

    /// @brief Emitted after the user clicked the button with any mouse-button
    ///
    /// A click is only emitted if the user released the mouse above the button
    /// and the button is enabled.
    ///
    /// @sa #leftClicked()
    void clicked(Qt::MouseButton button);

    /// @brief Emitted when the user presses the left mouse button.
    ///
    /// Avoid using this event where possible and use #leftClicked().
    void leftMousePress();

protected:
    void paintEvent(QPaintEvent * /*event*/) override;

    /// @brief Paint the contents to be shown below the button
    ///
    /// First, the content is painted, then hover and click effects and finally
    /// the border.
    /// The content is cached by default as it's assumed to be expensive to
    /// repaint and repaints can occur frequently.
    ///
    /// @sa #contentCacheEnabled()
    virtual void paintContent(QPainter &painter) = 0;

    /// @brief Paint hover and click effects.
    ///
    /// This is provided for custom setups that override #paintEvent().
    /// When possible, prefer #paintContent().
    void fancyPaint(QPainter &painter);

    /// @brief Indicate that the cache is invalid and needs to be repainted.
    ///
    /// This calls update().
    void invalidateContent();

    /// @brief Returns true if the content drawn in #paintContent() should be
    /// cached.
    ///
    /// For complex static content, this makes sense, as the button is often
    /// repainted when the user is hovering over it.
    ///
    /// By default, the cache is disabled.
    [[nodiscard]] bool contentCacheEnabled() const noexcept;

    /// @brief Setter for #contentCacheEnabled()
    ///
    /// When disabling the content-cache, the cached pixmap is destroyed.
    void setContentCacheEnabled(bool enabled);

    /// @brief Returns true if the content drawn in #paintContent() is fully
    /// opaque.
    ///
    /// If the content is fully opaque, the cached pixmap isn't filled with a
    /// transparent color, but left in the default state.
    ///
    /// @sa #setOpaqueContent()
    [[nodiscard]] bool opaqueContent() const noexcept;

    /// Setter for #opaqueContent()
    void setOpaqueContent(bool opaqueContent);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * /*event*/) override;
#else
    void enterEvent(QEvent * /*event*/) override;
#endif
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void onMouseEffectTimeout();
    void showMenu();

    void paintButton(QPainter &painter);

    QColor borderColor_;

    QPoint mousePos_;
    double hoverMultiplier_ = 0.0;

    std::unique_ptr<QMenu> menu_;

    QTimer effectTimer_;
    std::vector<ClickEffect> clickEffects_;
    std::optional<QColor> mouseEffectColor_;

    bool enabled_ = true;
    bool mouseOver_ = false;
    bool mouseDown_ = false;
    bool menuVisible_ = false;

    QPixmap cachedPixmap_;
    bool pixmapValid_ = false;
    bool cachePixmap_ = false;
    bool opaqueContent_ = false;
};

}  // namespace chatterino
