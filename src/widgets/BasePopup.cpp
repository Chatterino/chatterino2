#include "widgets/BasePopup.hpp"

namespace chatterino {

BasePopup::BasePopup(FlagsEnum<Flags> _flags, QWidget *parent)
    : BaseWindow(std::move(_flags), parent)
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
