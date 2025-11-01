#pragma once
#include "widgets/dialogs/font/FontFamilyWidget.hpp"
#include "widgets/dialogs/font/FontSizeWidget.hpp"
#include "widgets/dialogs/font/FontWeightWidget.hpp"
#include "widgets/dialogs/font/PreviewWidget.hpp"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>

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
