#pragma once

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {

class PreviewWidget : public QWidget
{
public:
    PreviewWidget(const QFont &startFont, QWidget *parent = nullptr);

    void setFont(const QFont &font);

    void paintEvent(QPaintEvent *event) override;

private:
    QFont font;
};

}  // namespace chatterino
