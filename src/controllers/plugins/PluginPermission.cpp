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
    switch (this->type)
    {
        case PluginPermission::Type::FilesystemRead:
            [[fallthrough]];
        case PluginPermission::Type::FilesystemWrite: {
            auto pathsObj = obj.value("paths");
            if (pathsObj.isUndefined())
            {
                this->errors.emplace_back(
                    QString("fs permission is missing paths")
                        .arg(jsontype.toString()));
                return;
            }
            if (!pathsObj.isArray())
            {
                QString type = magic_enum::enum_name(pathsObj.type()).data();
                this->errors.emplace_back(
                    QString("fs permission paths is not an array (its type "
                            "is %1)")
                        .arg(type));
                return;
            }

            auto pathsArr = pathsObj.toArray();
            for (int i = 0; i < pathsArr.size(); i++)
            {
                const auto &t = pathsArr.at(i);
                if (!t.isString())
                {
                    QString type = magic_enum::enum_name(t.type()).data();
                    this->errors.push_back(
                        QString("fs permission paths element #%1 is not a "
                                "string (its type is %2)")
                            .arg(i)
                            .arg(type));
                    return;
                }
                this->paths.push_back(t.toString());
            }
            break;
        }
    }
}

QString PluginPermission::toHtmlEscaped() const
{
    QString friendlyName;
    switch (this->type)
    {
        case PluginPermission::Type::FilesystemRead:
            friendlyName = "Read files:";
            [[fallthrough]];
        case PluginPermission::Type::FilesystemWrite: {
            if (friendlyName.isEmpty())
            {
                friendlyName = "Write or create files:";
            }
            QString pathsStr;
            for (size_t i = 0; i < this->paths.size(); i++)
            {
                if (i != 0)
                {
                    pathsStr += ", ";
                }
                pathsStr += this->paths.at(i).toHtmlEscaped();
            }

            return friendlyName + " " + pathsStr;
        }
        default:
            assert(false && "invalid PluginPermission type in toString()");
    }
}

}  // namespace chatterino
#endif
