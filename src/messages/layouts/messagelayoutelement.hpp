#pragma once

#include <QPoint>
#include <QRect>
#include <QString>

#include <boost/noncopyable.hpp>
#include <climits>

#include "messages/link.hpp"
#include "messages/messagecolor.hpp"
#include "singletons/fontmanager.hpp"

class QPainter;

namespace chatterino {
namespace messages {
struct MessageElement;
class Image;

namespace layouts {

struct MessageLayoutElement : boost::noncopyable
{
public:
    MessageLayoutElement(MessageElement &creator, const QSize &size);

    const QRect &getRect() const;
    MessageElement &getCreator() const;
    void setPosition(QPoint point);
    bool hasTrailingSpace() const;

    MessageLayoutElement *setTrailingSpace(bool value);
    MessageLayoutElement *setLink(const Link &link);

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
    QRect rect;
    Link link;
    //    bool isInNewLine;
    MessageElement &creator;
};

// IMAGE
class ImageLayoutElement : public MessageLayoutElement
{
public:
    ImageLayoutElement(MessageElement &creator, Image *image, const QSize &size);

protected:
    virtual void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    virtual int getSelectionIndexCount() override;
    virtual void paint(QPainter &painter) override;
    virtual void paintAnimated(QPainter &painter, int yOffset) override;
    virtual int getMouseOverIndex(const QPoint &abs) override;
    virtual int getXFromIndex(int index) override;

private:
    Image *image;
};

// TEXT
class TextLayoutElement : public MessageLayoutElement
{
public:
    TextLayoutElement(MessageElement &creator, QString &text, const QSize &size, QColor color,
                      FontStyle style, float scale);

protected:
    virtual void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    virtual int getSelectionIndexCount() override;
    virtual void paint(QPainter &painter) override;
    virtual void paintAnimated(QPainter &painter, int yOffset) override;
    virtual int getMouseOverIndex(const QPoint &abs) override;
    virtual int getXFromIndex(int index) override;

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
    TextIconLayoutElement(MessageElement &creator, const QString &line1, const QString &line2,
                          float scale, const QSize &size);

protected:
    virtual void addCopyTextToString(QString &str, int from = 0, int to = INT_MAX) const override;
    virtual int getSelectionIndexCount() override;
    virtual void paint(QPainter &painter) override;
    virtual void paintAnimated(QPainter &painter, int yOffset) override;
    virtual int getMouseOverIndex(const QPoint &abs) override;
    virtual int getXFromIndex(int index) override;

private:
    QString line1;
    QString line2;
    float scale;
};
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
