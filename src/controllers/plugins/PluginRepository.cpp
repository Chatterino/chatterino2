// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/PluginRepository.hpp"

#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"

using namespace Qt::Literals;

namespace chatterino {

const QString DEFAULT_PLUGIN_REPOSITORY = u"http://localhost:8080/"_s;

PluginRepository::PluginRepository(const QString &url)
{
    if (url == u"(default)")
    {
        this->baseURL = DEFAULT_PLUGIN_REPOSITORY;
    }
    else
    {
        this->baseURL = QUrl(url);
    }
}

bool PluginRepository::hasErrorOrWarnings() const
{
    return !this->error.isEmpty() || !this->warnings.isEmpty();
}

QString PluginRepository::getName() const
{
    return this->name;
}

QUrl PluginRepository::getBaseURL() const
{
    return this->baseURL;
}

QString PluginRepository::getError() const
{
    return this->error;
}

QString PluginRepository::getWarnings() const
{
    return this->warnings;
}

std::span<const RemotePluginPtr> PluginRepository::getPlugins() const
{
    return this->plugins;
}

void PluginRepository::load()
{
    if (this->baseURL.scheme() != u"https" && this->baseURL.scheme() != u"http")
    {
        this->error = u"URL scheme must be http(s)"_s;
        this->onLoaded.invoke();
        return;
    }

    NetworkRequest(this->fileUrl(u"meta.json"_s))
        .followRedirects(true)
        .timeout(10 * 1000)
        .onSuccess([weak = this->weak_from_this()](const NetworkResult &res) {
            auto self = weak.lock();
            if (!self)
            {
                return;
            }

            const auto json = res.parseJson();
            auto loadRes = self->loadFromJson(json);
            if (!loadRes)
            {
                assert(self->error.isEmpty());
                self->error = std::move(loadRes).error();
            }
            self->onLoaded.invoke();
        })
        .onError([weak = this->weak_from_this()](const NetworkResult &res) {
            auto self = weak.lock();
            if (!self)
            {
                return;
            }
            self->error = u"Failed to fetch metadata: " % res.formatError();
            self->onLoaded.invoke();
        })
        .execute();
}

QUrl PluginRepository::fileUrl(const QString &filename) const
{
    QUrl child(this->baseURL);
    if (filename.startsWith('/'))
    {
        child.setPath(filename, QUrl::StrictMode);
    }
    else
    {
        QString path = child.path(QUrl::FullyEncoded);
        if (!path.endsWith('/'))
        {
            path += '/';
        }
        path += filename;
        child.setPath(path, QUrl::StrictMode);
    }
    return child;
}

ExpectedStr<void> PluginRepository::loadFromJson(const QJsonObject &root)
{
    static const QRegularExpression nameRegex(uR"(^[\w +\.\-_':&]+$)"_s);
    static const QRegularExpression idRegex(uR"(^[\w+\.\-_@~]+$)"_s);

    const auto meta = root["metadata"_L1].toObject();
    if (meta.isEmpty())
    {
        return makeUnexpected(u"Invalid or missing 'metadata' object."_s);
    }

    this->name = meta["name"_L1].toString();
    if (this->name.isEmpty())
    {
        return makeUnexpected(u"Empty or missing 'name'"_s);
    }

    if (!nameRegex.match(this->name).hasMatch())
    {
        return makeUnexpected(
            u"Invalid name. Names must only contain alphanumeric characters, spaces, and selected special characters."_s);
    }

    auto pushWarning = [&](const auto &str) {
        if (!this->warnings.isEmpty())
        {
            this->warnings += '\n';
        }
        this->warnings += str;
    };

    const auto plugins = root["plugins"_L1].toArray();
    for (const auto jsonRef : plugins)
    {
        const auto pluginObj = jsonRef.toObject();
        auto pluginPtr = std::make_shared<RemotePlugin>(pluginObj);
        if (!idRegex.match(pluginPtr->id).hasMatch())
        {
            pushWarning("- ID '" % pluginPtr->id.toHtmlEscaped() %
                        "' is invalid.");
            continue;
        }
        if (!pluginPtr->meta.isValid())
        {
            QString warning = pluginPtr->id % u" contains invalid metadata: ";
            bool first = true;
            for (const auto &err : pluginPtr->meta.errors)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    warning += u", ";
                }
                warning += err;
            }
            pushWarning(warning);
            continue;
        }
        pluginPtr->meta.remoteBaseURL =
            this->baseURL.toEncoded(QUrl::FullyEncoded);

        if (pluginPtr->downloadPath.isEmpty())
        {
            pluginPtr->downloadPath = pluginPtr->id;
        }
        pluginPtr->downloadURL = this->fileUrl(pluginPtr->downloadPath);

        this->plugins.emplace_back(std::move(pluginPtr));
    }

    return {};
}

}  // namespace chatterino

#endif
