#include "controllers/highlights/HighlightBetaModel.hpp"

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "singletons/Resources.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightBetaModel::HighlightBetaModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(Column::COUNT, parent)
{
}

// turn a vector item into a model row
HighlightPhrase HighlightBetaModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightPhrase &original)
{
    qInfo() << "XXX: Get item from row";
    // In order for old messages to update their highlight color, we need to
    // update the highlight color here.
    auto highlightColor = original.getColor();
    *highlightColor =
        row[Column::Color]->data(Qt::DecorationRole).value<QColor>();

    auto highlightParams =
        get<HighlightData>(row[Column::Enabled]->data(DATA_ROLE));

    // TODO: Update all cells based on the new(?) HighlightData

    auto item = HighlightPhrase{
        // row[Column::Name]->data(Qt::DisplayRole).toString(),
        highlightParams.pattern,
        highlightParams.showInMentions,
        highlightParams.hasAlert,
        highlightParams.hasSound,
        highlightParams.isRegex,
        highlightParams.isCaseSensitive,
        highlightParams.soundUrl.toString(),
        highlightColor,
    };

    auto soundPixmap = [=] {
        if (item.hasCustomSound())
        {
            return getResources().buttons.music_note;
        }
        if (item.hasSound())
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    setBoolItem(row[Column::Enabled], item.isEnabled(), true, false);
    setIconItem(row[Column::Type], item.getType(), false);
    setStringItem(row[Column::Name], item.getPattern(), false, false);
    setIconItem(row[Column::Sound], soundPixmap, false);
    setColorItem(row[Column::Color], *item.getColor());

    return item;
}

// turns a row in the model into a vector item
void HighlightBetaModel::getRowFromItem(const HighlightPhrase &item,
                                        std::vector<QStandardItem *> &row)
{
    qInfo() << "XXX: Get row from item";
    HighlightData highlightData{
        .name = QString("Name of %1").arg(item.getPattern()),
        .pattern = item.getPattern(),
        .enabled = item.isEnabled(),
        .showInMentions = item.showInMentions(),
        .hasAlert = item.hasAlert(),
        .hasSound = item.hasSound(),
        .isRegex = item.isRegex(),
        .isCaseSensitive = item.isCaseSensitive(),
        .soundUrl = item.getSoundUrl(),
    };

    row[Column::Enabled]->setData(QVariant::fromValue(highlightData),
                                  DATA_ROLE);

    auto soundPixmap = [=] {
        if (item.hasCustomSound())
        {
            return getResources().buttons.music_note;
        }
        if (item.hasSound())
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    setBoolItem(row[Column::Enabled], item.isEnabled(), true, false);
    setIconItem(row[Column::Type], item.getType(), false);
    setStringItem(row[Column::Name], item.getPattern(), false, false);
    setIconItem(row[Column::Sound], soundPixmap, false);
    setColorItem(row[Column::Color], *item.getColor());
    setStringItem(row[Column::Configure], "Edit...");
    row[Column::Enabled]->setSelectable(false);
    row[Column::Enabled]->setEditable(false);
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
