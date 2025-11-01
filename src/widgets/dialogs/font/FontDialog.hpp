#pragma once

#include <QDialog>
#include <QFont>
#include <QWidget>

namespace chatterino {

class PreviewWidget;
class FontFamilyWidget;
class FontSizeWidget;
class FontWeightWidget;

class FontDialog : public QDialog
{
    Q_OBJECT

public:
    FontDialog(const QFont &startFont, QWidget *parent = nullptr);

    /// Gets the currently selected font.
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
