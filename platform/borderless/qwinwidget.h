/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/* 																										 */
/* File is originally from
 * https://github.com/qtproject/qt-solutions/tree/master/qtwinmigrate/src */
/* 																										 */
/* It has been modified to support borderless window (HTTTRANSPARENT) & to
 * remove pre Qt5 cruft          */
/* 																										 */
/* 																										 */

// Declaration of the QWinWidget classes

#ifndef QWINWIDGET_H
#define QWINWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "WinNativeWindow.h"
#include "widget.h"

class QWinWidget : public QWidget
{
    Q_OBJECT
public:
    QWinWidget();
    ~QWinWidget();

    void show();
    void center();
    void showCentered();
    void setGeometry(int x, int y, int w, int h);

    HWND getParentWindow() const;

public slots:
    void onMaximizeButtonClicked();
    void onMinimizeButtonClicked();
    void onCloseButtonClicked();

protected:
    void childEvent(QChildEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;

    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent *e) override;

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QVBoxLayout m_Layout;

    Widget *p_Widget;

    WinNativeWindow *p_ParentWinNativeWindow;
    HWND m_ParentNativeWindowHandle;

    HWND _prevFocus;
    bool _reenableParent;

    int BORDERWIDTH = 6;     // Adjust this as you wish for # of pixels on the
                             // edges to show resize handles
    int TOOLBARHEIGHT = 40;  // Adjust this as you wish for # of pixels from the
                             // top to allow dragging the window

    void saveFocus();
    void resetFocus();
};

#endif  // QWINWIDGET_H
