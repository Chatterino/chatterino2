#pragma once

#include <QString>

namespace chatterino {

class Image;

class ModerationAction
{
public:
    ModerationAction(chatterino::Image *image, const QString &action);
    ModerationAction(const QString &line1, const QString &line2, const QString &action);

    bool isImage() const;
    chatterino::Image *getImage() const;
    const QString &getLine1() const;
    const QString &getLine2() const;
    const QString &getAction() const;

private:
    bool _isImage;
    chatterino::Image *image;
    QString line1;
    QString line2;
    QString action;
};

}  // namespace chatterino
