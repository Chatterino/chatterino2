#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include <QJsonObject>
#    include <QString>

#    include <vector>

namespace chatterino {

struct PluginPermission {
    explicit PluginPermission(const QJsonObject &obj);

    enum class Type {
        FilesystemRead,
        FilesystemWrite,
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
