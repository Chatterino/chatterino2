#pragma once

#include "widgets/BaseWidget.hpp"

#include <QLayout>
#include <QToolButton>

namespace chatterino {

/**
 * This class provides a way to make any kind of QLayout collapsible. Users can
 * click a button to toggle visibility of the underlying layout.
 */
class ExpandableLayout : public BaseWidget
{
    Q_OBJECT

public:
    explicit ExpandableLayout(const QString &title, QWidget *parent = nullptr);

    /**
     * Attach a layout to be collapsed.
     *
     * If set, any previous layout will be deleted by this method. This widget
     * will take ownership of the passed layout.
     */
    void setContent(QLayout *layout);

public slots:
    /**
     * Toggle visibility of the attached layout.
     *
     * Set to `true` to expand the widget and `false` to hide it.
     */
    void toggle(bool expand);

signals:
    void buttonToggled(bool expanded);

private:
    // We use QToolButton because it has built-in support for nice arrows
    QToolButton *toggleButton_{};
    QWidget *content_{};
};

}  // namespace chatterino
