#include "widgets/dialogs/FontPicker.hpp"

#include <qabstractitemview.h>
#include <QFontDatabase>
#include <qlabel.h>
#include <qstringlistmodel.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlistview.h>

namespace {

constexpr auto SAMPLE_POINT_SIZE = 12;

}  // namespace

namespace chatterino {

FontPicker::FontPicker()
{
    auto *layout = new QVBoxLayout(this);

    auto *fontList = new QListView(this);
    layout->addWidget(fontList);

    auto *model = new QStringListModel(this);
    auto *selectionModel = new QItemSelectionModel(model);
    qInfo() << "font families:"
            << QFontDatabase::families(QFontDatabase::WritingSystem::Latin);
    QStringList fonts;
    for (const auto &font :
         QFontDatabase::families(QFontDatabase::WritingSystem::Latin))
    {
        if (QFontDatabase::isPrivateFamily(font))
        {
            continue;
        }

        fonts << font;
    }
    model->setStringList(fonts);

    fontList->setModel(model);
    fontList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fontList->setSelectionModel(selectionModel);

    auto *sample = new QLabel(this);
    sample->setText("forsen: This is some sample text xD");
    layout->addWidget(sample);

    QObject::connect(
        selectionModel, &QItemSelectionModel::currentChanged,
        [sample](const QModelIndex &selected, const QModelIndex &deselected) {
            (void)deselected;
            sample->setFont(QFontDatabase::font(selected.data().toString(),
                                                "Normal", SAMPLE_POINT_SIZE));
            qInfo() << "clicked font xd" << selected.data();
            // selcetion changed? xd
        });
}

}  // namespace chatterino
