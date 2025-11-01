#pragma once
#include "widgets/dialogs/font/FontFamilyWidget.hpp"
#include "widgets/dialogs/font/FontSizeWidget.hpp"
#include "widgets/dialogs/font/FontWeightWidget.hpp"
#include "widgets/dialogs/font/PreviewWidget.hpp"

#include <QDialog>
#include <QFont>
#include <QWidget>

namespace chatterino {

class FontDialog : public QDialog
{
    Q_OBJECT

public:
    FontDialog(const QFont &startFont, QWidget *parent = nullptr);
    QFont getSelected() const;

Q_SIGNALS:
    void applied();

private:
    void updatePreview();

    PreviewWidget *preview;
    FontFamilyWidget *familyW;
    FontSizeWidget *sizeW;
    FontWeightWidget *weightW;
};

}  // namespace chatterino
