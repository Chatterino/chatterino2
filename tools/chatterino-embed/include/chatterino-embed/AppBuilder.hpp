#pragma once

#include <chatterino-embed/Config.hpp>
#include <QObject>

#include <memory>
#include <optional>

namespace chatterino::embed {

class App;

class AppBuilderPrivate;
class CHATTERINO_EMBED_EXPORT AppBuilder : public QObject
{
    Q_OBJECT
public:
    AppBuilder(QObject *parent = nullptr);
    ~AppBuilder() override;

    /// The root directory for this app. Inside this directory, Chatterino will
    /// store all its data.
    ///
    /// If this is `std::nullopt`, the system default will be used.
    ///
    /// Default: `std::nullopt`.
    [[nodiscard]] std::optional<QString> rootDirectory() const;

    /// Set the root directory for this app.
    ///
    /// \see rootDirectory()
    void setRootDirectory(std::optional<QString> rootDirectory);

    /// Should the settings be saved when the app is destroyed?
    ///
    /// Default: false.
    [[nodiscard]] bool saveSettingsOnExit() const;

    /// Set whether settings should be saved when the app is destroyed.
    ///
    /// \see saveSettingsOnExit()
    void setSaveSettingsOnExit(bool value);

    /// Create an `App` instance.
    ///
    /// This will not have any parent. You are responsible for deleting it.
    [[nodiscard]] App *createApp();

private:
    std::unique_ptr<AppBuilderPrivate> private_;
};

}  // namespace chatterino::embed
