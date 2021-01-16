#pragma once

#include <QDialog>

namespace chatterino {

class SelectChannelFiltersDialog : public QDialog
{
public:
    SelectChannelFiltersDialog(const QList<QUuid> &previousSelection,
                               QWidget *parent = nullptr);

    const QList<QUuid> &getSelection() const;

private:
    QList<QUuid> currentSelection_;
};

}  // namespace chatterino
