#include "widgets/dialogs/switcher/QuickSwitcherPopup.hpp"

#include "Application.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/switcher/NewTabItem.hpp"
#include "widgets/dialogs/switcher/SwitchSplitItem.hpp"
#include "widgets/helper/NotebookTab.hpp"

namespace chatterino {

namespace {
    using namespace chatterino;

    QSet<SplitContainer *> openPages()
    {
        QSet<SplitContainer *> pages;

        auto &nb = getApp()->windows->getMainWindow().getNotebook();
        for (int i = 0; i < nb.getPageCount(); ++i)
        {
            pages.insert(static_cast<SplitContainer *>(nb.getPageAt(i)));
        }

        return pages;
    }
}  // namespace

const QSize QuickSwitcherPopup::MINIMUM_SIZE(500, 300);

QuickSwitcherPopup::QuickSwitcherPopup(QWidget *parent)
    : BasePopup(FlagsEnum<BaseWindow::Flags>{BaseWindow::Flags::Frameless,
                                             BaseWindow::Flags::TopMost},
                parent)
    , switcherModel_(this)
    , switcherItemDelegate_(this)
{
    this->setWindowFlag(Qt::Dialog);
    this->setActionOnFocusLoss(BaseWindow::ActionOnFocusLoss::Delete);
    this->setMinimumSize(QuickSwitcherPopup::MINIMUM_SIZE);

    this->initWidgets();

    this->setStayInScreenRect(true);
    const QRect geom = parent->geometry();
    // This places the popup in the middle of the parent widget
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                          this->size(), geom));

    this->themeChangedEvent();
}

QuickSwitcherPopup::~QuickSwitcherPopup()
{
}

void QuickSwitcherPopup::initWidgets()
{
    LayoutCreator<QWidget> creator(this->BaseWindow::getLayoutContainer());
    auto vbox = creator.setLayoutType<QVBoxLayout>();

    {
        auto lineEdit = vbox.emplace<QLineEdit>().assign(&this->ui_.searchEdit);
        lineEdit->setPlaceholderText("Jump to a channel or open a new one");
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
    for (auto *sc : openPages())
    {
        const QString &tabTitle = sc->getTab()->getTitle();
        const auto splits = sc->getSplits();

        // First, check for splits on this page
        for (auto *split : splits)
        {
            if (split->getChannel()->getName().contains(text,
                                                        Qt::CaseInsensitive))
            {
                auto item = std::make_unique<SwitchSplitItem>(split);
                this->switcherModel_.addItem(std::move(item));

                // We want to continue the outer loop so we need a goto
                goto nextPage;
            }
        }

        // Then check if tab title matches
        if (tabTitle.contains(text, Qt::CaseInsensitive))
        {
            auto item = std::make_unique<SwitchSplitItem>(sc);
            this->switcherModel_.addItem(std::move(item));
            continue;
        }

    nextPage:;
    }

    // Add item for opening a channel in a new tab
    if (!text.isEmpty())
    {
        auto item = std::make_unique<NewTabItem>(text);
        this->switcherModel_.addItem(std::move(item));
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

void QuickSwitcherPopup::themeChangedEvent()
{
    BasePopup::themeChangedEvent();

    const QString textCol = this->theme->window.text.name();
    const QString bgCol = this->theme->window.background.name();

    const QString selCol =
        (this->theme->isLightTheme()
             ? "#68B1FF"  // Copied from Theme::splits.input.styleSheet
             : this->theme->tabs.selected.backgrounds.regular.color().name());

    const QString listStyle =
        QString(
            "color: %1; background-color: %2; selection-background-color: %3")
            .arg(textCol)
            .arg(bgCol)
            .arg(selCol);

    this->ui_.searchEdit->setStyleSheet(this->theme->splits.input.styleSheet);
    this->ui_.list->setStyleSheet(listStyle);
}

}  // namespace chatterino
