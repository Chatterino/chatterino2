#pragma once

#include "controllers/highlights/SharedHighlight.hpp"
#include "widgets/BasePopup.hpp"

#include <qtmetamacros.h>
#include <QWidget>

namespace chatterino {

class HighlightsBetaConfigureDialog : public BasePopup
{
    Q_OBJECT

public:
    HighlightsBetaConfigureDialog(SharedHighlight _data, QWidget *parent);

    Q_SIGNAL void confirmed(SharedHighlight data);

private:
    SharedHighlight data;
};

}  // namespace chatterino
