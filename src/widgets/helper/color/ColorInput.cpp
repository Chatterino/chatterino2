#include "widgets/helper/color/ColorInput.hpp"

namespace {

// from qtools_p.h
int fromHex(char c) noexcept
{
    if (c >= '0' && c <= '9')
    {
        return int(c - '0');
    }
    if (c >= 'A' && c <= 'F')
    {
        return int(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f')
    {
        return int(c - 'a' + 10);
    }

    return -1;
}

QColor parseHexColor(const QString &text)
{
    if (text.length() == 5)  // #rgba
    {
        auto alphaHex = fromHex(text[4].toLatin1());
        QStringView v(text);
        v.chop(1);
        QColor col(v);
        col.setAlpha(alphaHex);
        return col;
    }
    QColor col(text);
    if (col.isValid() && text.length() == 9)  // #rrggbbaa
    {
        auto rgba = col.rgba();
        auto alpha = rgba & 0xff;
        QColor actual(rgba >> 8);
        actual.setAlpha((int)alpha);
        return actual;
    }
    return col;
}

}  // namespace

namespace chatterino {

ColorInput::ColorInput(QColor color, QWidget *parent)
    : QWidget(parent)
    , currentColor_(color)
    , hexValidator_(QRegularExpression(
          R"(^#([A-Fa-f\d]{3,4}|[A-Fa-f\d]{6}|[A-Fa-f\d]{8})$)"))
    , layout_(this)
{
    int row = 0;
    const auto initComponent = [&](Component &component, auto label,
                                   auto applyToColor) {
        component.lbl.setText(label);
        component.box.setRange(0, 255);
        QObject::connect(&component.box,
                         qOverload<int>(&QSpinBox::valueChanged), this,
                         [this, &component, applyToColor](int value) {
                             if (component.value == value)
                             {
                                 return;
                             }
                             applyToColor(this->currentColor_, value);

                             this->emitUpdate();
                         });
        this->layout_.addWidget(&component.lbl, row, 0);
        this->layout_.addWidget(&component.box, row, 1);
        row++;
    };

    initComponent(this->red_, "Red:", [](auto &color, int value) {
        color.setRed(value);
    });
    initComponent(this->green_, "Green:", [](auto &color, int value) {
        color.setGreen(value);
    });
    initComponent(this->blue_, "Blue:", [](auto &color, int value) {
        color.setBlue(value);
    });
    initComponent(this->alpha_, "Alpha:", [](auto &color, int value) {
        color.setAlpha(value);
    });

    this->hexLabel_.setText("Hex:");
    this->hexInput_.setValidator(&this->hexValidator_);
    QObject::connect(&this->hexInput_, &QLineEdit::editingFinished, [this]() {
        auto css = parseHexColor(this->hexInput_.text());
        if (!css.isValid() || this->currentColor_ == css)
        {
            return;
        }
        this->currentColor_ = css;
        this->emitUpdate();
    });
    this->layout_.addWidget(&this->hexLabel_, row, 0);
    this->layout_.addWidget(&this->hexInput_, row, 1);

    this->updateComponents();
}

void ColorInput::updateComponents()
{
    auto color = this->currentColor_.toRgb();
    const auto updateComponent = [](Component &component, auto getValue) {
        int value = getValue();
        if (component.value != value)
        {
            component.value = value;
            component.box.setValue(value);
        }
    };
    updateComponent(this->red_, [&]() {
        return color.red();
    });
    updateComponent(this->green_, [&]() {
        return color.green();
    });
    updateComponent(this->blue_, [&]() {
        return color.blue();
    });
    updateComponent(this->alpha_, [&]() {
        return color.alpha();
    });

    this->updateHex();
}

void ColorInput::updateHex()
{
    auto rgb = this->currentColor_.rgb();
    rgb <<= 8;
    rgb |= this->currentColor_.alpha();
    // we always need to update the CSS color
    this->hexInput_.setText(QStringLiteral("#%1").arg(rgb, 8, 16, QChar(u'0')));
}

QColor ColorInput::color() const
{
    return this->currentColor_;
}

void ColorInput::setColor(QColor color)
{
    if (this->currentColor_ == color)
    {
        return;
    }
    this->currentColor_ = color;
    this->updateComponents();
    // no emit, as we just got the updated color
}

void ColorInput::emitUpdate()
{
    this->updateComponents();
    // our components triggered this update, emit the new color
    this->colorChanged(this->currentColor_);
}

}  // namespace chatterino
