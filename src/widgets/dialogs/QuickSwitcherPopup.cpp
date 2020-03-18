#include "widgets/dialogs/QuickSwitcherPopup.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace chatterino {

namespace {
    using namespace chatterino;

    QSet<QuickSwitcherPopup::ChannelSplits> openedChannels()
    {
        QSet<QuickSwitcherPopup::ChannelSplits> channelSplits;
        auto &nb = getApp()->windows->getMainWindow().getNotebook();
        for (int i = 0; i < nb.getPageCount(); ++i)
        {
            auto *sc = static_cast<SplitContainer *>(nb.getPageAt(i));
            for (auto *split : sc->getSplits())
            {
                channelSplits.insert(
                    std::make_pair(split->getChannel(), split));
            }
        }

        return channelSplits;
    }
}  // namespace

QuickSwitcherPopup::QuickSwitcherPopup(QWidget *parent)
    : BasePopup(FlagsEnum<BaseWindow::Flags>{BaseWindow::Flags::Frameless,
                                             BaseWindow::Flags::TopMost},
                parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->initWidgets();
}

void QuickSwitcherPopup::initWidgets()
{
    LayoutCreator<QWidget> creator(this->BaseWindow::getLayoutContainer());
    auto vbox = creator.setLayoutType<QVBoxLayout>();

    {
        vbox.emplace<QLineEdit>().assign(&this->ui_.searchEdit);
        QObject::connect(this->ui_.searchEdit, &QLineEdit::textChanged, this,
                         &QuickSwitcherPopup::updateSuggestions);

        this->ui_.searchEdit->installEventFilter(this);
    }

    {
        vbox.emplace<QListWidget>().assign(&this->ui_.list);
    }
}

void QuickSwitcherPopup::updateSuggestions(const QString &text)
{
    this->ui_.list->clear();

    for (auto pairs : openedChannels())
    {
        auto chan = pairs.first;
        auto *split = pairs.second;
        if (chan->getName().contains(text, Qt::CaseInsensitive))
        {
            auto *item = new QListWidgetItem(
                split->getContainer()->getTab()->getTitle());
            item->setData(Qt::UserRole, QVariant::fromValue(split));
            this->ui_.list->addItem(item);
        }
    }

    if (!text.isEmpty())
    {
        this->ui_.list->addItem("Open channel \"" + text + "\"");
    }

    this->ui_.list->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
}

bool QuickSwitcherPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        const int curRow = this->ui_.list->currentRow();
        const int count = this->ui_.list->count();

        if (key == Qt::Key_Down || key == Qt::Key_Tab)
        {
            if (count <= 0)
                return true;

            const int newRow = (curRow + 1) % count;

            this->ui_.list->setCurrentRow(newRow,
                                          QItemSelectionModel::SelectCurrent);
            return true;
        }
        else if (key == Qt::Key_Up || key == Qt::Key_Backtab)
        {
            if (count <= 0)
                return true;

            int newRow = curRow - 1;
            if (newRow < 0)
                newRow += count;

            this->ui_.list->setCurrentRow(newRow,
                                          QItemSelectionModel::SelectCurrent);
            return true;
        }
        else if (key == Qt::Key_Enter || key == Qt::Key_Return)
        {
            if (count <= 0)
                return true;

            Split *selectedSplit = this->ui_.list->currentItem()
                                       ->data(Qt::UserRole)
                                       .value<Split *>();
            auto &nb = getApp()->windows->getMainWindow().getNotebook();
            nb.select(selectedSplit->getContainer());

            this->close();
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

}  // namespace chatterino
