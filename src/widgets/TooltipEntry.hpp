#pragma once

#include "messages/Image.hpp"

#include <QLabel>
#include <QWidget>

namespace chatterino {

class TooltipEntry : public QWidget
{
    Q_OBJECT

public:
    TooltipEntry(QWidget *parent = nullptr);
    TooltipEntry(ImagePtr image, const QString &text,
                 QWidget *parent = nullptr);
    TooltipEntry(ImagePtr image, const QString &text, int customWidth,
                 int customHeight, QWidget *parent = nullptr);

    void setImageScale(int w, int h);
    void setWordWrap(bool wrap);

    void setText(const QString &text);
    void setImage(ImagePtr image);
    void clearImage();
    bool refreshPixmap();

    bool animated() const;
    ImagePtr getImage() const;

private:
    QLabel *displayImage_;
    QLabel *displayText_;

    bool attemptRefresh{false};

    ImagePtr image_ = nullptr;
    int customImgWidth_ = 0;
    int customImgHeight_ = 0;
};

}  // namespace chatterino
