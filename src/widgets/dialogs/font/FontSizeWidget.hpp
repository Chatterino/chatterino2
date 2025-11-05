#pragma once

#include <QFont>
#include <QListWidget>
#include <QSpinBox>
#include <QWidget>

namespace chatterino {

class IntItem;

/// FontSizeWidget shows a subset of the possible font sizes in a list,
/// alongside a spinbox where the user can enter their exact font size.
class FontSizeWidget : public QWidget
{
    Q_OBJECT

public:
    FontSizeWidget(const QFont &startFont, QWidget *parent = nullptr);

    /// Gets the currently selected font size.
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    void setListSelected(int size);

    IntItem *customItem;  // displays the value from `edit`
    QListWidget *list;
    QSpinBox *edit;
};

}  // namespace chatterino
