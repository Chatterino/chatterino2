#pragma once

#include <QLayout>
#include <QStyle>

#include <vector>

namespace chatterino {

/// @brief A QLayout wrapping items
///
/// Similar to a box layout that wraps its items. It's not super optimized.
/// Some computations in #doLayout() could be cached.
///
/// This is based on the Qt flow layout example:
/// https://doc.qt.io/qt-6/qtwidgets-layouts-flowlayout-example.html
class FlowLayout : public QLayout
{
public:
    struct Options {
        int margin = -1;
        int hSpacing = -1;
        int vSpacing = -1;
    };

    explicit FlowLayout(QWidget *parent, Options options = {-1, -1, -1});
    explicit FlowLayout(Options options = {-1, -1, -1});

    ~FlowLayout() override;
    FlowLayout(const FlowLayout &) = delete;
    FlowLayout(FlowLayout &&) = delete;
    FlowLayout &operator=(const FlowLayout &) = delete;
    FlowLayout &operator=(FlowLayout &&) = delete;

    /// @brief Adds @a item to this layout
    ///
    /// Ownership of @a item is transferred. This method isn't usually called
    /// in application code (use addWidget/addLayout).
    /// See QLayout::addItem for more information.
    void addItem(QLayoutItem *item) override;

    /// @brief Adds a linebreak to this layout
    ///
    /// @param height Specifies the height of the linebreak
    void addLinebreak(int height = 0);

    /// @brief Spacing on the horizontal axis
    ///
    /// -1 if the default spacing for an item will be used.
    [[nodiscard]] int horizontalSpacing() const;

    /// Setter for #horizontalSpacing(). -1 to use defaults.
    void setHorizontalSpacing(int value);

    /// @brief Spacing on the vertical axis
    ///
    /// -1 if the default spacing for an item will be used.
    [[nodiscard]] int verticalSpacing() const;

    /// Setter for #verticalSpacing(). -1 to use defaults.
    void setVerticalSpacing(int value);

    /// From QLayout. This layout doesn't expand in any direction.
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;

    QSize minimumSize() const override;
    QSize sizeHint() const override;

    void setGeometry(const QRect &rect) override;

    int count() const override;
    QLayoutItem *itemAt(int index) const override;

    /// From QLayout. Ownership is transferred to the caller
    QLayoutItem *takeAt(int index) override;

private:
    /// @brief Computes the layout
    ///
    /// @param rect The area in which items can be layed out
    /// @param testOnly If set, items won't be moved, only the total height
    ///                 will be computed.
    /// @returns The total height including margins.
    int doLayout(const QRect &rect, bool testOnly) const;

    /// @brief Computes the default spacing based for items on the parent
    ///
    /// @param pm Either PM_LayoutHorizontalSpacing or PM_LayoutVerticalSpacing
    ///           for the respective direction.
    /// @returns The spacing in dp, -1 if there isn't any parent
    int defaultSpacing(QStyle::PixelMetric pm) const;

    /// Computes the spacing for @a item
    QSize getSpacing(QLayoutItem *item) const;

    std::vector<QLayoutItem *> itemList_;
    int hSpace_ = -1;
    int vSpace_ = -1;
    int lineSpacing_ = -1;
};

}  // namespace chatterino
