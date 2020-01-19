/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "widgets/helper/QColorPicker.hpp"

#include <qdrawutil.h>

/*
 * These classes are literally copied from the Qt source.
 * Unfortunately, they are private to the QColorDialog class so we cannot use
 * them directly.
 * If they become public at any point in the future, it should be possible to
 * replace every include of this header with the respective includes for the
 * QColorPicker, QColorLuminancePicker, and QColSpinBox classes.
 */
namespace chatterino {

int QColorLuminancePicker::y2val(int y)
{
    int d = height() - 2 * coff - 1;
    return 255 - (y - coff) * 255 / d;
}

int QColorLuminancePicker::val2y(int v)
{
    int d = height() - 2 * coff - 1;
    return coff + (255 - v) * d / 255;
}

QColorLuminancePicker::QColorLuminancePicker(QWidget *parent)
    : QWidget(parent)
{
    hue = 100;
    val = 100;
    sat = 100;
    pix = 0;
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QColorLuminancePicker::~QColorLuminancePicker()
{
    delete pix;
}

void QColorLuminancePicker::mouseMoveEvent(QMouseEvent *m)
{
    setVal(y2val(m->y()));
}

void QColorLuminancePicker::mousePressEvent(QMouseEvent *m)
{
    setVal(y2val(m->y()));
}

void QColorLuminancePicker::setVal(int v)
{
    if (val == v)
        return;
    val = qMax(0, qMin(v, 255));
    delete pix;
    pix = 0;
    repaint();
    emit newHsv(hue, sat, val);
}

//receives from a hue,sat chooser and relays.
void QColorLuminancePicker::setCol(int h, int s)
{
    setCol(h, s, val);
    emit newHsv(h, s, val);
}

QSize QColorLuminancePicker::sizeHint() const
{
    return QSize(LUMINANCE_PICKER_WIDTH, LUMINANCE_PICKER_HEIGHT);
}

void QColorLuminancePicker::paintEvent(QPaintEvent *)
{
    int w = width() - 5;
    QRect r(0, foff, w, height() - 2 * foff);
    int wi = r.width() - 2;
    int hi = r.height() - 2;
    if (!pix || pix->height() != hi || pix->width() != wi)
    {
        delete pix;
        QImage img(wi, hi, QImage::Format_RGB32);
        int y;
        uint *pixel = (uint *)img.scanLine(0);
        for (y = 0; y < hi; y++)
        {
            uint *end = pixel + wi;
            std::fill(pixel, end,
                      QColor::fromHsv(hue, sat, y2val(y + coff)).rgb());
            pixel = end;
        }
        pix = new QPixmap(QPixmap::fromImage(img));
    }
    QPainter p(this);
    p.drawPixmap(1, coff, *pix);
    const QPalette &g = palette();
    qDrawShadePanel(&p, r, g, true);
    p.setPen(g.windowText().color());
    p.setBrush(g.windowText());
    QPolygon a;
    int y = val2y(val);
    a.setPoints(3, w, y, w + 5, y + 5, w + 5, y - 5);
    p.eraseRect(w, 0, 5, height());
    p.drawPolygon(a);
}

void QColorLuminancePicker::setCol(int h, int s, int v)
{
    val = v;
    hue = h;
    sat = s;
    delete pix;
    pix = 0;
    repaint();
}

QPoint QColorPicker::colPt()
{
    QRect r = contentsRect();
    return QPoint((360 - hue) * (r.width() - 1) / 360,
                  (255 - sat) * (r.height() - 1) / 255);
}

int QColorPicker::huePt(const QPoint &pt)
{
    QRect r = contentsRect();
    return 360 - pt.x() * 360 / (r.width() - 1);
}

int QColorPicker::satPt(const QPoint &pt)
{
    QRect r = contentsRect();
    return 255 - pt.y() * 255 / (r.height() - 1);
}

void QColorPicker::setCol(const QPoint &pt)
{
    setCol(huePt(pt), satPt(pt));
}

QColorPicker::QColorPicker(QWidget *parent)
    : QFrame(parent)
    , crossVisible(true)
{
    hue = 0;
    sat = 0;
    setCol(150, 255);
    setAttribute(Qt::WA_NoSystemBackground);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QColorPicker::~QColorPicker()
{
}

void QColorPicker::setCrossVisible(bool visible)
{
    if (crossVisible != visible)
    {
        crossVisible = visible;
        update();
    }
}

QSize QColorPicker::sizeHint() const
{
    return QSize(COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
}

void QColorPicker::setCol(int h, int s)
{
    int nhue = qMin(qMax(0, h), 359);
    int nsat = qMin(qMax(0, s), 255);
    if (nhue == hue && nsat == sat)
        return;
    QRect r(colPt(), QSize(20, 20));
    hue = nhue;
    sat = nsat;
    r = r.united(QRect(colPt(), QSize(20, 20)));
    r.translate(contentsRect().x() - 9, contentsRect().y() - 9);
    repaint(r);
}

void QColorPicker::mouseMoveEvent(QMouseEvent *m)
{
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol(p);
    emit newCol(hue, sat);
}

void QColorPicker::mousePressEvent(QMouseEvent *m)
{
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol(p);
    emit newCol(hue, sat);
}

void QColorPicker::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    drawFrame(&p);
    QRect r = contentsRect();
    p.drawPixmap(r.topLeft(), pix);
    if (crossVisible)
    {
        QPoint pt = colPt() + r.topLeft();
        p.setPen(Qt::black);
        p.fillRect(pt.x() - 9, pt.y(), 20, 2, Qt::black);
        p.fillRect(pt.x(), pt.y() - 9, 2, 20, Qt::black);
    }
}

void QColorPicker::resizeEvent(QResizeEvent *ev)
{
    QFrame::resizeEvent(ev);
    int w = width() - frameWidth() * 2;
    int h = height() - frameWidth() * 2;
    QImage img(w, h, QImage::Format_RGB32);
    int x, y;
    uint *pixel = (uint *)img.scanLine(0);
    for (y = 0; y < h; y++)
    {
        const uint *end = pixel + w;
        x = 0;
        while (pixel < end)
        {
            QPoint p(x, y);
            QColor c;
            c.setHsv(huePt(p), satPt(p), 200);
            *pixel = c.rgb();
            ++pixel;
            ++x;
        }
    }
    pix = QPixmap::fromImage(img);
}

QColSpinBox::QColSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    this->setRange(0, 255);
}

void QColSpinBox::setValue(int i)
{
    const QSignalBlocker blocker(this);
    QSpinBox::setValue(i);
}

}  // namespace chatterino
