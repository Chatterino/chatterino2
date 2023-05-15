#pragma once

#include "providers/autocomplete/AutocompleteSource.hpp"
#include "widgets/listview/GenericListModel.hpp"

#include <QObject>
#include <QString>

namespace chatterino {

class AutocompleteModel : public GenericListModel
{
public:
    AutocompleteModel(QObject *parent = nullptr);

    void setSource(std::unique_ptr<AutocompleteSource> source);
    bool hasSource() const;

    void updateResults(const QString &query, size_t maxCount = 0);

private:
    std::unique_ptr<AutocompleteSource> source_{};
};

};  // namespace chatterino
