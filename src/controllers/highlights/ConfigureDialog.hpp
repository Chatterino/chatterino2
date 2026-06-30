#pragma once

#include "controllers/highlights/types/All.hpp"
#include "util/DisplayBadge.hpp"
#include "widgets/BasePopup.hpp"

#include <qtmetamacros.h>
#include <QWidget>

#include <memory>
#include <optional>

namespace chatterino::highlights {

class ConfigureDialog : public BasePopup
{
    Q_OBJECT

public:
    ConfigureDialog(AllHighlights _data, QWidget *parent);

    Q_SIGNAL void confirmed(AllHighlights data);

private:
    AllHighlights data;
};

}  // namespace chatterino::highlights
