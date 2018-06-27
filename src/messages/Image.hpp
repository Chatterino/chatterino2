#pragma once

#include <QPixmap>
#include <QString>
#include <boost/noncopyable.hpp>

#include <atomic>

namespace chatterino {

class Image : public QObject, boost::noncopyable
{
public:
    explicit Image(const QString &_url, qreal _scale = 1, const QString &_name = "",
                   const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                   bool isHat = false);

    explicit Image(QPixmap *_currentPixmap, qreal _scale = 1, const QString &_name = "",
                   const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                   bool isHat = false);
    ~Image();

    const QPixmap *getPixmap();
    qreal getScale() const;
    const QString &getUrl() const;
    const QString &getName() const;
    const QString &getCopyString() const;
    const QString &getTooltip() const;
    const QMargins &getMargin() const;
    bool isAnimated() const;
    bool isHat() const;
    int getWidth() const;
    int getScaledWidth() const;
    int getHeight() const;
    int getScaledHeight() const;

    void setCopyString(const QString &newCopyString);

private:
    struct FrameData {
        QPixmap *image;
        int duration;
    };

    static bool loadedEventQueued;

    QPixmap *currentPixmap = nullptr;
    QPixmap *loadedPixmap = nullptr;
    std::vector<FrameData> allFrames;
    int currentFrame = 0;
    int currentFrameOffset = 0;

    QString url;
    QString name;
    QString copyString;
    QString tooltip;
    bool animated = false;
    QMargins margin;
    bool ishat;
    qreal scale;

    bool isLoading = false;
    std::atomic<bool> isLoaded{false};

    void loadImage();
    void gifUpdateTimout();
};

}  // namespace chatterino
