// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/font/FontSettingDialog.hpp"

#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep

namespace chatterino {

FontSettingDialog::FontSettingDialog(QStringSetting &family, IntSetting &size,
                                     IntSetting &weight, QWidget *parent)
    : FontDialog({family, size, weight}, parent)
    , familySetting(family)
    , sizeSetting(size)
    , weightSetting(weight)
    , oldFamily(family)
    , oldSize(size)
    , oldWeight(weight)
{
    this->setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(this, &FontDialog::applied, this,
                     &FontSettingDialog::setSettings);

    QObject::connect(this, &FontDialog::rejected, this,
                     &FontSettingDialog::restoreSettings);

    QObject::connect(this, &FontDialog::accepted, this,
                     &FontSettingDialog::setSettings);
}

void FontSettingDialog::setSettings()
{
    const auto selected = this->getSelected();

    this->familySetting.setValue(selected.family());
    this->sizeSetting.setValue(selected.pointSize());
    this->weightSetting.setValue(selected.weight());
}

void FontSettingDialog::restoreSettings()
{
    this->familySetting.setValue(this->oldFamily);
    this->sizeSetting.setValue(this->oldSize);
    this->weightSetting.setValue(this->oldWeight);
}

}  // namespace chatterino
