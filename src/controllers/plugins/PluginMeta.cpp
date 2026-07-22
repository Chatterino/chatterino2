// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginMeta.hpp"

#    include "util/Expected.hpp"
#    include "util/QMagicEnum.hpp"

#    include <QJsonArray>
#    include <QJsonObject>
#    include <QJsonValue>
#    include <QRegularExpression>

using namespace Qt::Literals;

namespace {

using namespace chatterino;

bool validatePath(const QString &path)
{
    static const QRegularExpression regex(uR"(^[\w_\-+\./]+$)"_s);

    return regex.matchView(path).hasMatch() && !path.contains(u"..") &&
           !path.contains(u"//") && !path.startsWith('/') &&
           !path.endsWith('/');
}

}  // namespace

namespace chatterino {

// NOLINTBEGIN(clazy-reserve-candidates)
PluginMeta::PluginMeta(const QJsonObject &obj)
{
    auto homepageObj = obj.value("homepage");
    if (homepageObj.isString())
    {
        this->homepage = homepageObj.toString();
    }
    else if (!homepageObj.isUndefined())
    {
        auto type = qmagicenum::enumName(homepageObj.type());
        this->errors.emplace_back(
            QString("homepage is defined but is not a string (its type is %1)")
                .arg(type));
    }
    auto nameObj = obj.value("name");
    if (nameObj.isString())
    {
        this->name = nameObj.toString();
    }
    else
    {
        auto type = qmagicenum::enumName(nameObj.type());
        this->errors.emplace_back(
            QString("name is not a string (its type is %1)").arg(type));
    }

    auto descrObj = obj.value("description");
    if (descrObj.isString())
    {
        this->description = descrObj.toString();
    }
    else
    {
        auto type = qmagicenum::enumName(descrObj.type());
        this->errors.emplace_back(
            QString("description is not a string (its type is %1)").arg(type));
    }

    auto authorsObj = obj.value("authors");
    if (authorsObj.isArray())
    {
        auto authorsArr = authorsObj.toArray();
        for (int i = 0; i < authorsArr.size(); i++)
        {
            const auto &t = authorsArr.at(i);
            if (!t.isString())
            {
                auto type = qmagicenum::enumName(t.type());
                this->errors.push_back(
                    QString("authors element #%1 is not a string (it is a %2)")
                        .arg(i)
                        .arg(type));
                break;
            }
            this->authors.push_back(t.toString());
        }
    }
    else
    {
        auto type = qmagicenum::enumName(authorsObj.type());
        this->errors.emplace_back(
            QString("authors is not an array (its type is %1)").arg(type));
    }

    auto licenseObj = obj.value("license");
    if (licenseObj.isString())
    {
        this->license = licenseObj.toString();
    }
    else
    {
        auto type = qmagicenum::enumName(licenseObj.type());
        this->errors.emplace_back(
            QString("license is not a string (its type is %1)").arg(type));
    }

    auto verObj = obj.value("version");
    if (verObj.isString())
    {
        auto v = semver::from_string_noexcept(verObj.toString().toStdString());
        if (v.has_value())
        {
            this->version = v.value();
        }
        else
        {
            this->errors.emplace_back("unable to parse version (use semver)");
            this->version = semver::version(0, 0, 0);
        }
    }
    else
    {
        auto type = qmagicenum::enumName(verObj.type());
        this->errors.emplace_back(
            QString("version is not a string (its type is %1)").arg(type));
        this->version = semver::version(0, 0, 0);
    }
    auto permsObj = obj.value("permissions");
    if (!permsObj.isUndefined())
    {
        if (!permsObj.isArray())
        {
            auto type = qmagicenum::enumName(permsObj.type());
            this->errors.emplace_back(
                QString("permissions is not an array (its type is %1)")
                    .arg(type));
            return;
        }

        auto permsArr = permsObj.toArray();
        for (int i = 0; i < permsArr.size(); i++)
        {
            const auto &t = permsArr.at(i);
            if (!t.isObject())
            {
                auto type = qmagicenum::enumName(t.type());
                this->errors.push_back(QString("permissions element #%1 is not "
                                               "an object (its type is %2)")
                                           .arg(i)
                                           .arg(type));
                return;
            }
            auto parsed = PluginPermission(t.toObject());
            if (parsed.isValid())
            {
                // ensure no invalid permissions slip through this
                this->permissions.push_back(parsed);
            }
            else
            {
                for (const auto &err : parsed.errors)
                {
                    this->errors.push_back(
                        QString("permissions element #%1: %2").arg(i).arg(err));
                }
            }
        }
    }

    auto tagsObj = obj.value("tags");
    if (!tagsObj.isUndefined())
    {
        if (!tagsObj.isArray())
        {
            auto type = qmagicenum::enumName(tagsObj.type());
            this->errors.emplace_back(
                QString("tags is not an array (its type is %1)").arg(type));
            return;
        }

        auto tagsArr = tagsObj.toArray();
        for (int i = 0; i < tagsArr.size(); i++)
        {
            const auto &t = tagsArr.at(i);
            if (!t.isString())
            {
                auto type = qmagicenum::enumName(t.type());
                this->errors.push_back(
                    QString("tags element #%1 is not a string (its type is %2)")
                        .arg(i)
                        .arg(type));
                return;
            }
            this->tags.push_back(t.toString());
        }
    }

    auto remote = obj["remote"];
    if (!remote.isUndefined())
    {
        if (remote.isString())
        {
            this->remoteBaseURL = remote.toString();
        }
        else
        {
            this->errors.emplace_back(
                u"Remote URL is not a string (its type is %1)"_s.arg(
                    qmagicenum::enumName(remote.type())));
            return;
        }
    }

    auto files = obj["files"];
    if (!files.isUndefined())
    {
        if (!files.isArray())
        {
            this->errors.emplace_back(
                u"\"files\" is not an array (its type is %1)"_s.arg(
                    qmagicenum::enumName(files.type())));
            return;
        }
        const auto arr = files.toArray();
        for (qsizetype i = 0; i < arr.size(); ++i)
        {
            auto v = arr[i];
            if (!v.isString())
            {
                this->errors.emplace_back(
                    u"\"files\"[%1] is not a string (its type is %2)"_s.arg(
                        QString::number(i), qmagicenum::enumName(v.type())));
                continue;
            }
            QString str = v.toString();
            if (!validatePath(str))
            {
                this->errors.emplace_back(
                    u"\"files\"[%1] contains invalid characters"_s.arg(
                        QString::number(i)));
                continue;
            }
            this->files.emplace_back(str);
        }
    }
}
// NOLINTEND(clazy-reserve-candidates)

bool PluginMeta::isRelatedTo(const PluginMeta &other, QString *conflicts) const
{
    if (other.remoteBaseURL != this->remoteBaseURL)
    {
        if (conflicts)
        {
            *conflicts =
                u"Remote URLs differ (current: \"%1\", other: \"%2\")"_s.arg(
                    other.remoteBaseURL, this->remoteBaseURL);
        }
        return false;
    }

    return true;
}

QJsonObject PluginMeta::toJson() const
{
    auto map = [](const auto &range, auto &&cb) {
        QJsonArray arr;
        for (const auto &it : range)
        {
            arr.append(std::invoke(cb, it));
        }
        return arr;
    };
    auto ifNotEmpty = [](auto &&value) -> QJsonValue {
        if (!value.isEmpty())
        {
            return value;
        }
        return QJsonValue::Undefined;
    };

    return {
        {"homepage"_L1, ifNotEmpty(this->homepage)},
        {"name"_L1, this->name},
        {"description"_L1, this->description},
        {"authors"_L1, map(this->authors, std::identity{})},
        {"license"_L1, this->license},
        {"version"_L1, QString::fromStdString(this->version.to_string())},
        {"permissions"_L1, map(this->permissions, &PluginPermission::toJson)},
        {"tags"_L1, ifNotEmpty(map(this->tags, std::identity{}))},
        {"private"_L1, ifNotEmpty(this->privateFields)},
        {"remote"_L1, ifNotEmpty(this->remoteBaseURL)},
        // Forget "files".
    };
}

}  // namespace chatterino

#endif
