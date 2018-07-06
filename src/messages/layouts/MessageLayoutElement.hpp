#pragma once

#include <QPoint>
#include <QRect>
#include <QString>

#include <boost/noncopyable.hpp>
#include <climits>

#include "messages/Link.hpp"
#include "messages/MessageColor.hpp"
#include "singletons/Fonts.hpp"

class QPainter;

namespace chatterino {
class MessageElement;
class Image;

class MessageLayoutElement : boost::noncopyable
{
public:
    MessageLayoutElement(MessageElement &creator_, const QSize &size);
    virtual ~MessageLayoutElement();

    const QRect &getRect() const;
    MessageElement &getCreator() const;
    void setPosition(QPoint point);
    bool hasTrailingSpace() const;

    MessageLayoutElement *setTrailingSpace(bool value);
    MessageLayoutElement *setLink(const Link &link_);

    virtual void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const = 0;
    virtual int getSelectionIndexCount() = 0;
    virtual void paint(QPainter &painter) = 0;
    virtual void paintAnimated(QPainter &painter, int yOffset) = 0;
    virtual int getMouseOverIndex(const QPoint &abs) = 0;
    virtual int getXFromIndex(int index) = 0;
    const Link &getLink() const;

protected:
    bool trailingSpace = true;

private:
    QRect rect_;
    Link link_;
    MessageElement &creator_;
};

// IMAGE
class ImageLayoutElement : public MessageLayoutElement
{
public:
    ImageLayoutElement(MessageElement &creator_, Image *image, const QSize &size);

protected:
    void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    int getSelectionIndexCount() override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) override;
    int getXFromIndex(int index) override;

private:
    Image *image;
};

// TEXT
class TextLayoutElement : public MessageLayoutElement
{
public:
    TextLayoutElement(MessageElement &creator_, QString &text, const QSize &size, QColor color,
                      FontStyle style, float scale);

protected:
    void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    int getSelectionIndexCount() override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) override;
    int getXFromIndex(int index) override;

private:
    QString text;
    QColor color;
    FontStyle style;
    float scale;
};

// TEXT ICON
// two lines of text (characters) in the size of a normal chat badge
class TextIconLayoutElement : public MessageLayoutElement
{
public:
    TextIconLayoutElement(MessageElement &creator_, const QString &line1, const QString &line2,
                          float scale, const QSize &size);

protected:
    void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    int getSelectionIndexCount() override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) override;
    int getXFromIndex(int index) override;

private:
    float scale;
    QString line1;
    QString line2;
};

}  // namespace chatterino
