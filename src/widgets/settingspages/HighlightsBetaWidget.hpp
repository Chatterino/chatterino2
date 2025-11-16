#pragma once

#include "widgets/BaseWidget.hpp"

#include <QModelIndex>
#include <qstandarditemmodel.h>
#include <qtableview.h>

namespace chatterino {

class HighlightsBetaConfigureDialog;

class HighlightsBetaWidget : public BaseWidget
{
public:
    HighlightsBetaWidget();

private:
    void openConfigureDialog(QTableView *view, const QModelIndex &index);

    HighlightsBetaConfigureDialog *configureDialog = nullptr;
};

}  // namespace chatterino
