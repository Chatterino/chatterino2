#pragma once

#include "widgets/BaseWidget.hpp"

#include <QModelIndex>

namespace chatterino {

class HighlightsBetaConfigureDialog;

class HighlightsBetaWidget : public BaseWidget
{
public:
    HighlightsBetaWidget();

private:
    void openConfigureDialog(const QModelIndex &index);

    HighlightsBetaConfigureDialog *configureDialog = nullptr;
};

}  // namespace chatterino
