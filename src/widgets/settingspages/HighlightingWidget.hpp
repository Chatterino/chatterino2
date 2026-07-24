#pragma once

#include "widgets/BaseWidget.hpp"

#include <QModelIndex>

#include <cstdint>

class QTableView;

namespace chatterino {

namespace highlights {

class ConfigureDialog;

}  // namespace highlights

class HighlightingWidget : public BaseWidget
{
public:
    HighlightingWidget();

private:
    enum class ConfigureCloseBehaviour : std::uint8_t {
        Cancel,
        Remove,
    } configureCloseBehaviour = ConfigureCloseBehaviour::Cancel;

    void openConfigureDialog(QTableView *view, int vectorIndex,
                             ConfigureCloseBehaviour newCloseBehaviour);
    void openConfigureDialog(QTableView *view, const QModelIndex &index,
                             ConfigureCloseBehaviour newCloseBehaviour);

    highlights::ConfigureDialog *configureDialog = nullptr;
};

}  // namespace chatterino
