#pragma once

#include "common/ChatterinoSetting.hpp"
#include "widgets/dialogs/font/FontDialog.hpp"

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

class FontSettingDialog : public FontDialog
{
    Q_OBJECT

public:
    FontSettingDialog(QStringSetting &family, IntSetting &size,
                      IntSetting &weight, QWidget *parent = nullptr);

private:
    void setSettings();

    QStringSetting &familyOpt;
    IntSetting &sizeOpt;
    IntSetting &weightOpt;

    QString oldFamily;
    int oldSize;
    int oldWeight;
    bool needRestore{};
};

}  // namespace chatterino
