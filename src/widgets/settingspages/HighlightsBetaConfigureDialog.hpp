#pragma once

#include "controllers/highlights/HighlightPhrase.hpp"
#include "widgets/BasePopup.hpp"

#include <qtmetamacros.h>
#include <QWidget>

namespace chatterino {

class HighlightsBetaConfigureDialog : public BasePopup
{
    Q_OBJECT

public:
    HighlightsBetaConfigureDialog(HighlightData _data, QWidget *parent);

    Q_SIGNAL void confirmed(HighlightData data);

private:
    HighlightData data;
};

}  // namespace chatterino
