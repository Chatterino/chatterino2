/*#pragma once

class QString;
class QByteArray;
class QObject;
class QJsonObject;

namespace chatterino
{
    class PrivateNetRequest;

    enum class Caching {
        NoCache,
        PrefereCache,
        PrefereNoCache,
    };

    enum class NetRequestType {
        Get,
        Post,
        Put,
        Delete,
    };

    class HttpResult
    {
        QJsonObject toJson();
    };

    class HttpRequest
    {
    public:
        HttpRequest(QObject* receiver, const QString& url, NetRequestType type);
        ~HttpRequest();

        void setCaching(Caching);
        void setBody(const QByteArray&);
        void setHeader(const QString& name, const QString& value);
        void setTimeout(int msg);

        void execute();

    private:
        PrivateNetRequest* this_{};
    };

    class TwitchApiV5Request : public HttpRequest
    {
    };
}  // namespace chatterino
*/
