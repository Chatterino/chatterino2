#include "settingsdialogtab.h"
#include "QPainter"

SettingsDialogTab::SettingsDialogTab(QWidget *parent, QString label, QImage& image)
    : QWidget(parent),
      image(image)
{
    this->label = label;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    int a = (height() - image.width()) / 2;

    painter.drawImage(a, a, image);

    a = a + a + image.width();

    painter.drawText(QRect(a, 0, width() - a, height()), label, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}
