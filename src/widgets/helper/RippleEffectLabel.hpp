#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/Label.hpp"
#include "widgets/helper/RippleEffectButton.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class RippleEffectLabel : public RippleEffectButton
{
public:
    explicit RippleEffectLabel(BaseWidget *parent = nullptr, int spacing = 6);

    SignalLabel &getLabel()
    {
        return this->label_;
    }

private:
    QHBoxLayout hbox_;
    SignalLabel label_;
};

class RippleEffectLabel2 : public RippleEffectButton
{
public:
    explicit RippleEffectLabel2(BaseWidget *parent = nullptr, int padding = 6);

    Label &getLabel();

private:
    Label label_;
};

}  // namespace chatterino
