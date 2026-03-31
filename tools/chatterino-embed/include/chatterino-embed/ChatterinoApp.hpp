#pragma once

#include <chatterino-embed/ChatterinoConfig.hpp>
#include <QObject>

#include <memory>

class QWidget;
class QString;

namespace chatterino::embed {

class ChatterinoAppPrivate;
class CHATTERINO_EMBED_EXPORT ChatterinoApp : public QObject
{
    Q_OBJECT
public:
    ~ChatterinoApp() override;

    QWidget *createSplitFromData(QWidget *parent, QByteArrayView data);
    QWidget *createEmptySplit(QWidget *parent);

    QByteArray serializeSplit(QWidget *split);

protected:
    ChatterinoApp(ChatterinoAppPrivate *private_, QObject *parent = nullptr);

private:
    std::unique_ptr<ChatterinoAppPrivate> private_;

    friend ChatterinoAppPrivate;
};

}  // namespace chatterino::embed
