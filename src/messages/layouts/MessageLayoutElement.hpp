#pragma once

#include <QPoint>
#include <QRect>
#include <QString>
#include <boost/noncopyable.hpp>
#include <climits>

#include "common/FlagsEnum.hpp"
#include "messages/Link.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"

class QPainter;

namespace chatterino {
class MessageElement;
class Image;
using ImagePtr = std::shared_ptr<Image>;
enum class FontStyle : uint8_t;

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
    MessageLayoutElement *setText(const QString &text_);

    virtual void addCopyTextToString(QString &str, int from = 0,
                                     int to = INT_MAX) const = 0;
    virtual int getSelectionIndexCount() const = 0;
    virtual void paint(QPainter &painter) = 0;
    virtual void paintAnimated(QPainter &painter, int yOffset) = 0;
    virtual int getMouseOverIndex(const QPoint &abs) const = 0;
    virtual int getXFromIndex(int index) = 0;
    const Link &getLink() const;
    const QString &getText() const;
    FlagsEnum<MessageElementFlag> getFlags() const;

protected:
    bool trailingSpace = true;

private:
    QString text_;
    QRect rect_;
    Link link_;
    MessageElement &creator_;
};

// IMAGE
class ImageLayoutElement : public MessageLayoutElement
{
public:
    ImageLayoutElement(MessageElement &creator, ImagePtr image,
                       const QSize &size);

protected:
    void addCopyTextToString(QString &str, int from = 0,
                             int to = INT_MAX) const override;
    int getSelectionIndexCount() const override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) const override;
    int getXFromIndex(int index) override;

private:
    ImagePtr image_;
};

// TEXT
class TextLayoutElement : public MessageLayoutElement
{
public:
    TextLayoutElement(MessageElement &creator_, QString &text,
                      const QSize &size, QColor color, FontStyle style,
                      float scale);

    void listenToLinkChanges();

protected:
    void addCopyTextToString(QString &str, int from = 0,
                             int to = INT_MAX) const override;
    int getSelectionIndexCount() const override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) const override;
    int getXFromIndex(int index) override;

private:
    QColor color;
    FontStyle style;
    float scale;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

// TEXT ICON
// two lines of text (characters) in the size of a normal chat badge
class TextIconLayoutElement : public MessageLayoutElement
{
public:
    TextIconLayoutElement(MessageElement &creator_, const QString &line1,
                          const QString &line2, float scale, const QSize &size);

protected:
    void addCopyTextToString(QString &str, int from = 0,
                             int to = INT_MAX) const override;
    int getSelectionIndexCount() const override;
    void paint(QPainter &painter) override;
    void paintAnimated(QPainter &painter, int yOffset) override;
    int getMouseOverIndex(const QPoint &abs) const override;
    int getXFromIndex(int index) override;

private:
    float scale;
    QString line1;
    QString line2;
};

}  // namespace chatterino
