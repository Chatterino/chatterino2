#pragma once

#include <QString>

namespace chatterino {

class Image;

class ModerationAction
{
public:
    ModerationAction(messages::Image *image, const QString &action);
    ModerationAction(const QString &line1, const QString &line2, const QString &action);

    bool isImage() const;
    messages::Image *getImage() const;
    const QString &getLine1() const;
    const QString &getLine2() const;
    const QString &getAction() const;

private:
    bool _isImage;
    messages::Image *image;
    QString line1;
    QString line2;
    QString action;
};

}  // namespace chatterino
