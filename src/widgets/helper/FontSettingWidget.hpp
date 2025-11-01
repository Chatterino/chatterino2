#pragma once

#include "util/SignalListener.hpp"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
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

class FontSettingWidget : public QWidget
{
public:
    FontSettingWidget(QWidget *parent = nullptr);

private:
    void updateCurrentLabel();
    void showDialog();

    QLabel *currentLabel;
    SignalListener listener;
};

}  // namespace chatterino
