#pragma once

#include <QWidget>

namespace chatterino {

class Split;

namespace embed {

class Split : public QWidget
{
    Q_OBJECT

public:
    Split(QWidget *parent = nullptr);

    void deserializeData(QByteArrayView data);
    QByteArray serializeData();

    QString channelName() const;

Q_SIGNALS:
    void closeRequested();
    void channelChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    chatterino::Split *split_;
};

}  // namespace embed
}  // namespace chatterino
