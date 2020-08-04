#pragma once

#include "common/Channel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"
#include "widgets/dialogs/switcher/SwitcherItemDelegate.hpp"
#include "widgets/splits/Split.hpp"

#include <functional>

namespace chatterino {

class QuickSwitcherPopup : public BasePopup
{
public:
    // TODO(leon): Replace this with a custom type in order to be able to
    // overload equality operator
    using ChannelSplits = std::pair<ChannelPtr, Split *>;

    /**
     * @brief   Construct a new QuickSwitcherPopup.
     *
     * @param   parent  Parent widget of the popup. The popup will be placed
     *                  in the center of the parent widget.
     */
    explicit QuickSwitcherPopup(QWidget *parent = nullptr);

    ~QuickSwitcherPopup();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void updateSuggestions(const QString &text);

private:
    struct {
        QLineEdit *searchEdit{};
        QListView *list{};
    } ui_;

    QuickSwitcherModel switcherModel_;
    SwitcherItemDelegate switcherItemDelegate_;

    QSet<ChannelSplits> openSplits_;

    void initWidgets();
};

inline uint qHash(const ChannelPtr &key)
{
    return qHash(key->getName());
}

}  // namespace chatterino
