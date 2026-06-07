#include "controllers/highlights/HighlightBetaModel.hpp"

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/SharedHighlight.hpp"
#include "singletons/Resources.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightBetaModel::HighlightBetaModel(QObject *parent)
    : SignalVectorModel<SharedHighlight>(Column::COUNT, parent)
{
}

// turn a vector item into a model row
SharedHighlight HighlightBetaModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const SharedHighlight &original)
{
    qInfo() << "XXX: Get item from row";
    auto item = get<SharedHighlight>(row[Column::Enabled]->data(DATA_ROLE));

    // TODO: Update all cells based on the new(?) SharedHighlight

    auto soundPixmap = [=] {
        if (item.willPlayCustomSound())
        {
            return getResources().buttons.music_note;
        }
        if (item.willPlayAnySound())
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    if (item.enabled)
    {
        setStringItem(row[Column::Enabled], "Enabled", false);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);
    }
    setIconItem(row[Column::Type], item.getType(), false);
    setStringItem(row[Column::Name], item.pattern, false, false);
    setIconItem(row[Column::Sound], soundPixmap, false);

    return item;
}

// turns a row in the model into a vector item
void HighlightBetaModel::getRowFromItem(const SharedHighlight &item,
                                        std::vector<QStandardItem *> &row)
{
    qInfo() << "XXX: Get row from item";

    row[Column::Enabled]->setData(QVariant::fromValue(item), DATA_ROLE);

    auto soundPixmap = [=] {
        if (item.willPlayCustomSound())
        {
            return getResources().buttons.music_note;
        }
        if (item.willPlayAnySound())
        {
            return getResources().buttons.music_note_2;
        }

        return getResources().buttons.speaker_mute;
    }();

    if (item.enabled)
    {
        setStringItem(row[Column::Enabled], "Enabled", false);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);
    }
    setIconItem(row[Column::Type], item.getType());
    setStringItem(row[Column::Name], item.pattern, false);
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
