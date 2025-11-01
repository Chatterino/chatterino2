#include "widgets/dialogs/font/FontSettingDialog.hpp"

#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep

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

FontSettingDialog::FontSettingDialog(QStringSetting &family, IntSetting &size,
                                     IntSetting &weight, QWidget *parent)
    : FontDialog({family, size, weight}, parent)
    , familyOpt(family)
    , sizeOpt(size)
    , weightOpt(weight)
    , oldFamily(family)
    , oldSize(size)
    , oldWeight(weight)
{
    QObject::connect(this, &FontDialog::applied, [this] {
        this->needRestore = true;
        this->setSettings();
    });
    QObject::connect(this, &FontDialog::rejected, [this] {
        if (this->needRestore)
        {
            this->familyOpt = this->oldFamily;
            this->sizeOpt = this->oldSize;
            this->weightOpt = this->oldWeight;
        }
    });
    QObject::connect(this, &FontDialog::accepted, [this] {
        this->setSettings();
    });
}

void FontSettingDialog::setSettings()
{
    QFont selected = this->getSelected();
    this->familyOpt = selected.family();
    this->sizeOpt = selected.pointSize();
    this->weightOpt = selected.weight();
}
}  // namespace chatterino
