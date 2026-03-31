#pragma once

#include <chatterino-embed/Config.hpp>
#include <QObject>

#include <memory>

namespace chatterino::embed {

class App;

class AppBuilderPrivate;
class CHATTERINO_EMBED_EXPORT AppBuilder : public QObject
{
    Q_OBJECT
public:
    AppBuilder(QObject *parent = nullptr);
    ~AppBuilder() override;

    QString rootDirectory() const;
    void setRootDirectory(const QString &rootDirectory);

    bool saveSettingsOnExit() const;
    void setSaveSettingsOnExit(bool value);

    App *createApp();

private:
    std::unique_ptr<AppBuilderPrivate> private_;
};

}  // namespace chatterino::embed
