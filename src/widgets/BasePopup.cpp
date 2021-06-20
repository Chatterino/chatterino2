#include "widgets/BasePopup.hpp"

#include <QKeyEvent>

namespace chatterino {

BasePopup::BasePopup(FlagsEnum<Flags> _flags, QWidget *parent)
    : BaseWindow(_flags | Dialog, parent)
{
}

void BasePopup::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
    {
        this->close();
        return;
    }

    BaseWindow::keyPressEvent(e);
}

}  // namespace chatterino
