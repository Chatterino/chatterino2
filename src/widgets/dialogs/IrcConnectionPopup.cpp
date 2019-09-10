#include "IrcConnectionPopup.hpp"

#include "providers/irc/Irc2.hpp"
#include "util/LayoutHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QHBoxLayout>
#include <QTableView>

namespace chatterino {

IrcConnectionPopup::IrcConnectionPopup(QWidget *parent)
    : BaseWindow(parent, BaseWindow::Flags::EnableCustomFrame)
{
    this->setWindowTitle("Edit Irc Connections");

    // view
    auto view =
        new EditableModelView(Irc::getInstance().newConnectionModel(this));

    view->setTitles({"host", "port", "ssl", "user", "nick", "password"});
    view->getTableView()->horizontalHeader()->resizeSection(0, 140);
    view->getTableView()->horizontalHeader()->resizeSection(1, 30);
    view->getTableView()->horizontalHeader()->resizeSection(2, 30);

    this->setScaleIndependantSize(800, 500);

    view->addButtonPressed.connect([] {
        auto unique = IrcConnection_{};
        unique.id = Irc::getInstance().uniqueId();
        Irc::getInstance().connections.appendItem(unique);
    });

    // init layout
    this->getLayoutContainer()->setLayout(makeLayout<QHBoxLayout>({view}));
}

}  // namespace chatterino
