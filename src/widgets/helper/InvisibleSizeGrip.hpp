#pragma once

#include <QSizeGrip>

namespace chatterino {

class InvisibleSizeGrip : public QSizeGrip
{
public:
    explicit InvisibleSizeGrip(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

}  // namespace chatterino
