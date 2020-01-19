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
#pragma once

#include <QSpinBox>

namespace chatterino {

/*
 * These classes are literally copied from the Qt source.
 * Unfortunately, they are private to the QColorDialog class so we cannot use
 * them directly.
 * If they become public at any point in the future, it should be possible to
 * replace every include of this header with the respective includes for the
 * QColorPicker, QColorLuminancePicker, and QColSpinBox classes.
 */
class QColorPicker : public QFrame
{
    Q_OBJECT
public:
    QColorPicker(QWidget *parent);
    ~QColorPicker();
    void setCrossVisible(bool visible);

public slots:
    void setCol(int h, int s);

signals:
    void newCol(int h, int s);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    int hue;
    int sat;
    QPoint colPt();
    int huePt(const QPoint &pt);
    int satPt(const QPoint &pt);
    void setCol(const QPoint &pt);
    QPixmap pix;
    bool crossVisible;
};

static const int COLOR_PICKER_WIDTH = 220;
static const int COLOR_PICKER_HEIGHT = 200;

class QColorLuminancePicker : public QWidget
{
    Q_OBJECT
public:
    QColorLuminancePicker(QWidget *parent = 0);
    ~QColorLuminancePicker();

public slots:
    void setCol(int h, int s, int v);
    void setCol(int h, int s);

signals:
    void newHsv(int h, int s, int v);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    enum { foff = 3, coff = 4 };  //frame and contents offset
    int val;
    int hue;
    int sat;
    int y2val(int y);
    int val2y(int val);
    void setVal(int v);
    QPixmap *pix;
};

static const int LUMINANCE_PICKER_WIDTH = 25;
static const int LUMINANCE_PICKER_HEIGHT = COLOR_PICKER_HEIGHT;

class QColSpinBox : public QSpinBox
{
public:
    QColSpinBox(QWidget *parent);

    void setValue(int i);
};

}  // namespace chatterino
