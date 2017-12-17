#pragma once

#include <QPixmap>
#include <QString>

namespace chatterino {
namespace messages {

class LazyLoadedImage : public QObject
{
public:
    LazyLoadedImage() = delete;

    explicit LazyLoadedImage(const QString &_url, qreal _scale = 1, const QString &_name = "",
                             const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                             bool isHat = false);

    explicit LazyLoadedImage(QPixmap *_currentPixmap, qreal _scale = 1, const QString &_name = "",
                             const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                             bool isHat = false);

    const QPixmap *getPixmap();
    qreal getScale() const;
    const QString &getUrl() const;
    const QString &getName() const;
    const QString &getTooltip() const;
    const QMargins &getMargin() const;
    bool getAnimated() const;
    bool isHat() const;
    int getWidth() const;
    int getScaledWidth() const;
    int getHeight() const;
    int getScaledHeight() const;

private:
    struct FrameData {
        QPixmap *image;
        int duration;
    };

    QPixmap *currentPixmap;
    std::vector<FrameData> allFrames;
    int currentFrame = 0;
    int currentFrameOffset = 0;

    QString url;
    QString name;
    QString tooltip;
    bool animated = false;
    QMargins margin;
    bool ishat;
    qreal scale;

    bool isLoading;

    void loadImage();
    void gifUpdateTimout();
};

}  // namespace messages
}  // namespace chatterino
