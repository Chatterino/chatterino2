#include "controllers/highlights/HighlightBetaModel.hpp"

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "singletons/Resources.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightBetaModel::HighlightBetaModel(QObject *parent)
    : SignalVectorModel<AllHighlights>(Column::COUNT, parent)
{
}

// turn a vector item into a model row
AllHighlights HighlightBetaModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const AllHighlights &original)
{
    qInfo() << "XXX: Get item from row";
    auto item = get<AllHighlights>(row[Column::Enabled]->data(DATA_ROLE));

    // TODO: Update all cells based on the new(?) AllHighlights

    auto soundPixmap = [=] {
        if (std::visit(
                [](auto &&v) {
                    return v.willPlayCustomSound();
                },
                item))
        {
            return getResources().buttons.music_note;
        }
        if (std::visit(
                [](auto &&v) {
                    return v.willPlayAnySound();
                },
                item))
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    if (std::visit(
            [](auto &&v) {
                return v.isEnabled();
            },
            item))
    {
        setStringItem(row[Column::Enabled], "Enabled", false);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);
    }
    setIconItem(row[Column::Type],
                std::visit(
                    [](auto &&v) {
                        return v.getType();
                    },
                    item),
                false);
    setStringItem(row[Column::Name],
                  std::visit(
                      [](auto &&v) {
                          return v.pattern;
                      },
                      item),
                  false, false);
    setIconItem(row[Column::Sound], soundPixmap, false);

    return item;
}

// turns a row in the model into a vector item
void HighlightBetaModel::getRowFromItem(const AllHighlights &item,
                                        std::vector<QStandardItem *> &row)
{
    qInfo() << "XXX: Get row from item";

    row[Column::Enabled]->setData(QVariant::fromValue(item), DATA_ROLE);

    auto soundPixmap = [=] {
        if (std::visit(
                [](auto &&v) {
                    return v.willPlayCustomSound();
                },
                item))
        {
            return getResources().buttons.music_note;
        }
        if (std::visit(
                [](auto &&v) {
                    return v.willPlayAnySound();
                },
                item))
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    if (std::visit(
            [](auto &&v) {
                return v.isEnabled();
            },
            item))
    {
        setStringItem(row[Column::Enabled], "Enabled", false);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);
    }
    setIconItem(row[Column::Type],
                std::visit(
                    [](auto &&v) {
                        return v.getType();
                    },
                    item),
                false);
    setStringItem(row[Column::Name],
                  std::visit(
                      [](auto &&v) {
                          return v.pattern;
                      },
                      item),
                  false, false);
    setIconItem(row[Column::Sound], soundPixmap);
    // row[Column::Enabled]->setSelectable(false);
    // row[Column::Enabled]->setEditable(false);
}

void HighlightBetaModel::afterInit()
{
    // Highlight settings for own username
}

void HighlightBetaModel::customRowSetData(
    const std::vector<QStandardItem *> &row, int column, const QVariant &value,
    int role, int rowIndex)
{
}

}  // namespace chatterino
