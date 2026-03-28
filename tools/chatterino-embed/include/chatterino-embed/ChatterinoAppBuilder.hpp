#pragma once

#include <chatterino-embed/ChatterinoConfig.hpp>
#include <QObject>

#include <memory>

namespace chatterino::embed {

class ChatterinoApp;
class ChatterinoEmbedInstanceImpl;

class ChatterinoAppBuilderPrivate;
class CHATTERINO_EMBED_EXPORT ChatterinoAppBuilder : public QObject
{
    Q_OBJECT
public:
    ChatterinoAppBuilder(QObject *parent = nullptr);
    ~ChatterinoAppBuilder() override;

    QString rootDirectory() const;
    void setRootDirectory(const QString &rootDirectory);

    bool saveSettingsOnExit() const;
    void setSaveSettingsOnExit(bool value);

    ChatterinoApp *createApp();

private:
    std::unique_ptr<ChatterinoAppBuilderPrivate> private_;

    friend ChatterinoEmbedInstanceImpl;
};

}  // namespace chatterino::embed
