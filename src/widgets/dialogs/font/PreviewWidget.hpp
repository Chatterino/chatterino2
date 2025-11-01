#pragma once
#include <QFont>

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
