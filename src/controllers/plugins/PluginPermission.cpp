#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginPermission.hpp"

#    include <magic_enum/magic_enum.hpp>
#    include <QJsonObject>

namespace chatterino {

PluginPermission::PluginPermission(const QJsonObject &obj)
{
    auto jsontype = obj.value("type");
    if (!jsontype.isString())
    {
        QString tn = magic_enum::enum_name(jsontype.type()).data();
        this->errors.emplace_back(QString("permission type is defined but is "
                                          "not a string (its type is %1)")
                                      .arg(tn));
    }
    auto strtype = jsontype.toString().toStdString();
    auto opt = magic_enum::enum_cast<PluginPermission::Type>(
        strtype, magic_enum::case_insensitive);
    if (!opt.has_value())
    {
        this->errors.emplace_back(QString("permission type is an unknown (%1)")
                                      .arg(jsontype.toString()));
        return;  // There is no more data to get, we don't know what to do
    }
    this->type = opt.value();
}

QString PluginPermission::toHtml() const
{
    switch (this->type)
    {
        case PluginPermission::Type::FilesystemRead:
            return "Read files in its data directory";
        case PluginPermission::Type::FilesystemWrite:
            return "Write to or create files in its data directory";
        default:
            assert(false && "invalid PluginPermission type in toHtml()");
            return "shut up compiler, this never happens";
    }
}

}  // namespace chatterino
#endif
