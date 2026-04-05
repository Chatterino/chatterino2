// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/font/FontFamilyWidget.hpp"

#include <QBoxLayout>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>

namespace chatterino {

namespace {

/// Gets a list of available font families available on this system
QStringList getFontFamilies()
{
    QStringList families = QFontDatabase::families();
    families.removeIf(QFontDatabase::isPrivateFamily);
    return families;
}

}  // namespace

FontFamilyWidget::FontFamilyWidget(const QFont &startFont, QWidget *parent)
    : QWidget(parent)
    , list(new QListView)
    , model(new QStringListModel(getFontFamilies(), this))
    , proxy(new QSortFilterProxyModel(this))
{
    auto *layout = new QVBoxLayout;
    auto *header = new QHBoxLayout;
    auto *search = new QLineEdit;

    this->setLayout(layout);

    this->list->setModel(this->proxy);
    this->list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    this->proxy->setSourceModel(this->model);
    this->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    this->setSelected(startFont.family());

    layout->addLayout(header);
    layout->addWidget(this->list);
    layout->setContentsMargins(0, 0, 0, 0);

    header->addWidget(new QLabel("Font"));
    header->addWidget(search);

    search->setPlaceholderText("Search...");

    QObject::connect(search, &QLineEdit::textChanged, this->proxy,
                     &QSortFilterProxyModel::setFilterFixedString);

    QObject::connect(
        this->list->selectionModel(), &QItemSelectionModel::currentChanged,
        this, [this](const QModelIndex &proxyIndex, const QModelIndex &) {
            if (proxyIndex.isValid())
            {
                Q_EMIT this->selectedChanged();
            }
        });
}

void FontFamilyWidget::setSelected(const QString &family)
{
    qsizetype row = this->model->stringList().indexOf(family);
    if (row >= 0)
    {
        QModelIndex modelIndex = this->model->index(static_cast<int>(row));
        QModelIndex proxyIndex = this->proxy->mapFromSource(modelIndex);

        this->list->selectionModel()->setCurrentIndex(
            proxyIndex, QItemSelectionModel::ClearAndSelect);
    }
}

QString FontFamilyWidget::getSelected() const
{
    QModelIndex proxyIndex = this->list->currentIndex();
    if (!proxyIndex.isValid())
    {
        return {};
    }

    QModelIndex modelIndex = this->proxy->mapToSource(proxyIndex);
    return this->model->data(modelIndex).toString();
}

}  // namespace chatterino
