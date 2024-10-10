#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include <QJsonObject>
#    include <QString>

#    include <vector>

namespace chatterino {

struct PluginPermission {
    explicit PluginPermission(const QJsonObject &obj);
    // This is for tests
    PluginPermission() = default;

    enum class Type {
        FilesystemRead,
        FilesystemWrite,
        Network,
    };
    Type type;
    std::vector<QString> errors;

    bool isValid() const
    {
        return this->errors.empty();
    }

    QString toHtml() const;
};

}  // namespace chatterino
#endif
