#include "widgets/dialogs/switcher/QuickSwitcherPopup.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/switcher/NewTabItem.hpp"
#include "widgets/dialogs/switcher/SwitchSplitItem.hpp"
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
    , switcherModel_(this)
    , switcherItemDelegate_(this)
    , openSplits_(openedChannels())
{
    this->setWindowFlag(Qt::Dialog);
    this->setActionOnFocusLoss(BaseWindow::ActionOnFocusLoss::Delete);

    this->initWidgets();

    this->setStayInScreenRect(true);
    const QRect geom = parent->geometry();
    // This places the popup in the middle of the parent widget
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                          this->size(), geom));
}

QuickSwitcherPopup::~QuickSwitcherPopup()
{
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
        vbox.emplace<QListView>().assign(&this->ui_.list);
        this->ui_.list->setSelectionMode(QAbstractItemView::SingleSelection);
        this->ui_.list->setSelectionBehavior(QAbstractItemView::SelectItems);
        this->ui_.list->setModel(&this->switcherModel_);
        this->ui_.list->setItemDelegate(&this->switcherItemDelegate_);

        /*
         * I also tried handling key events using the according slots but
         * it lead to all kind of problems that did not occur with the
         * eventFilter approach.
         */
        QObject::connect(
            this->ui_.list, &QListView::clicked, this,
            [this](const QModelIndex &index) {
                auto *item = AbstractSwitcherItem::fromVariant(index.data());
                item->action();
                this->close();
            });
    }
}

void QuickSwitcherPopup::updateSuggestions(const QString &text)
{
    this->switcherModel_.clear();

    // Add items for navigating to different splits
    for (auto pairs : this->openSplits_)
    {
        ChannelPtr chan = pairs.first;
        Split *split = pairs.second;
        if (chan->getName().contains(text, Qt::CaseInsensitive))
        {
            const QString title = split->getContainer()->getTab()->getTitle();

            SwitchSplitItem *item = new SwitchSplitItem(title, split);
            this->switcherModel_.addItem(item);
        }
    }

    // Add item for opening a channel in a new tab
    if (!text.isEmpty())
    {
        NewTabItem *item = new NewTabItem(text);
        this->switcherModel_.addItem(item);
    }

    const auto &startIdx = this->switcherModel_.index(0);
    this->ui_.list->setCurrentIndex(startIdx);

    /*
     * Timeout interval 0 means the call will be delayed until all window events
     * have been processed (cf. https://doc.qt.io/qt-5/qtimer.html#interval-prop).
     */
    QTimer::singleShot(0, [this] { this->adjustSize(); });
}

bool QuickSwitcherPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();

        const QModelIndex &curIdx = this->ui_.list->currentIndex();
        const int curRow = curIdx.row();
        const int count = this->switcherModel_.rowCount(curIdx);

        if (key == Qt::Key_Down || key == Qt::Key_Tab)
        {
            if (count <= 0)
                return true;

            const int newRow = (curRow + 1) % count;

            this->ui_.list->setCurrentIndex(curIdx.siblingAtRow(newRow));
            return true;
        }
        else if (key == Qt::Key_Up || key == Qt::Key_Backtab)
        {
            if (count <= 0)
                return true;

            int newRow = curRow - 1;
            if (newRow < 0)
                newRow += count;

            this->ui_.list->setCurrentIndex(curIdx.siblingAtRow(newRow));
            return true;
        }
        else if (key == Qt::Key_Enter || key == Qt::Key_Return)
        {
            if (count <= 0)
                return true;

            const auto index = this->ui_.list->currentIndex();
            auto *item = AbstractSwitcherItem::fromVariant(index.data());

            item->action();

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
