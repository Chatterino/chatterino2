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
#include "widgets/listview/GenericListView.hpp"

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

    this->installEventFilter(this->ui_.list);
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
    }

    {
        auto listView = vbox.emplace<GenericListView>().assign(&this->ui_.list);
        listView->setModel(&this->switcherModel_);

        QObject::connect(listView.getElement(),
                         &GenericListView::closeRequested, this, [this] {
                             this->close();
                         });

        this->ui_.searchEdit->installEventFilter(listView.getElement());
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
    QTimer::singleShot(0, [this] {
        this->adjustSize();
    });
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
    this->ui_.list->refreshTheme(*this->theme);
}

}  // namespace chatterino
