// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

#define SETTINGS_PAGE_WIDGET_BOILERPLATE(type, parent) \
    class type : public parent                         \
    {                                                  \
        using parent::parent;                          \
                                                       \
    public:                                            \
        bool greyedOut{};                              \
                                                       \
    protected:                                         \
        void paintEvent(QPaintEvent *e) override       \
        {                                              \
            parent::paintEvent(e);                     \
                                                       \
            if (this->greyedOut)                       \
            {                                          \
                QPainter painter(this);                \
                QColor color = QColor("#222222");      \
                color.setAlphaF(0.7F);                 \
                painter.fillRect(this->rect(), color); \
            }                                          \
        }                                              \
    };

namespace chatterino {

// S* widgets are the same as their Q* counterparts,
// but they can be greyed out and will be if you search.
SETTINGS_PAGE_WIDGET_BOILERPLATE(SCheckBox, QCheckBox)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SLabel, QLabel)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SComboBox, QComboBox)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SPushButton, QPushButton)

}  // namespace chatterino
