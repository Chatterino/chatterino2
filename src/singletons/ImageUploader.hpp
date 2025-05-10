#pragma once

#include <QMimeData>
#include <QMutex>
#include <QString>
#include <QStringView>

#include <memory>
#include <queue>

namespace chatterino {

class ResizingTextEdit;
class Channel;
class NetworkResult;
using ChannelPtr = std::shared_ptr<Channel>;

}  // namespace chatterino

namespace chatterino::imageuploader::detail {

/// Traverses the JSON value with a pattern where each key is separated by dots.
///
/// If the pattern doesn't match, an empty string is returned.
///
/// **Example**:
///
/// - JSON: `{"foo": {"bar": [1, "baz"]}}`
/// - pattern: `foo.bar.1`
/// - return value: `"baz"`
QString getJSONValue(QJsonValue responseJson, QStringView jsonPattern);

/// Interpolates `pattern` with the JSON response.
/// **Example**:
///
/// - response: `{"foo": {"bar": [1, "baz", "qox"]}}`
/// - pattern: `https://example.com/{foo.bar.1}.{foo.bar.2}`
/// - return value: `"https://example.com/baz.qox"`
QString getLinkFromResponse(const NetworkResult &response, QString pattern);

}  // namespace chatterino::imageuploader::detail

namespace chatterino {

struct RawImageData {
    QByteArray data;
    QString format;
    QString filePath;
};

class ImageUploader final
{
public:
    /**
     * Tries to get the image(s) from the given QMimeData
     *
     * If no images were found, the second value in the pair will contain an error message
     */
    std::pair<std::queue<RawImageData>, QString> getImages(
        const QMimeData *source) const;

    void upload(std::queue<RawImageData> images, ChannelPtr channel,
                QPointer<ResizingTextEdit> outputTextEdit);

private:
    void sendImageUploadRequest(RawImageData imageData, ChannelPtr channel,
                                QPointer<ResizingTextEdit> textEdit);

    // This is called from the onSuccess handler of the NetworkRequest in sendImageUploadRequest
    void handleSuccessfulUpload(const NetworkResult &result,
                                QString originalFilePath, ChannelPtr channel,
                                QPointer<ResizingTextEdit> textEdit);
    void handleFailedUpload(const NetworkResult &result, ChannelPtr channel);

    void logToFile(const QString &originalFilePath, const QString &imageLink,
                   const QString &deletionLink, ChannelPtr channel);

    // These variables are only used from the main thread.
    QMutex uploadMutex_;
    std::queue<RawImageData> uploadQueue_;
};
}  // namespace chatterino
