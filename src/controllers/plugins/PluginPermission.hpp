#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include <vector>

namespace chatterino {

struct PluginPermission {
    enum class Type {
        FilesystemRead,
        FilesystemWrite,
    };
    Type type;
    std::vector<QString> paths;

    std::vector<QString> errors;
    bool isValid() const
    {
        return this->errors.empty();
    }
    QString toHtmlEscaped() const;

    explicit PluginPermission(const QJsonObject &obj);
};

}  // namespace chatterino
#endif
