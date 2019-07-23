#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/Label.hpp"
#include "widgets/helper/Button.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace AB_NAMESPACE {

class EffectLabel : public Button
{
public:
    explicit EffectLabel(BaseWidget *parent = nullptr, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->label_;
    }

private:
    QHBoxLayout hbox_;
    SignalLabel label_;
};

class EffectLabel2 : public Button
{
public:
    explicit EffectLabel2(BaseWidget *parent = nullptr, int padding = 6);

    Label &getLabel();

private:
    Label label_;
};

}  // namespace AB_NAMESPACE
