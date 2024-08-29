#pragma once

#include <QObject>

#include <memory>

namespace chatterino {

class IStreamerMode : public QObject
{
    Q_OBJECT

public:
    IStreamerMode() = default;
    ~IStreamerMode() override = default;
    IStreamerMode(const IStreamerMode &) = delete;
    IStreamerMode(IStreamerMode &&) = delete;
    IStreamerMode &operator=(const IStreamerMode &) = delete;
    IStreamerMode &operator=(IStreamerMode &&) = delete;

    [[nodiscard]] virtual bool isEnabled() const = 0;

    virtual void start() = 0;

signals:
    void changed(bool enabled);
};

class StreamerModePrivate;
class StreamerMode : public IStreamerMode
{
public:
    StreamerMode();
    ~StreamerMode() override;
    StreamerMode(const StreamerMode &) = delete;
    StreamerMode(StreamerMode &&) = delete;
    StreamerMode &operator=(const StreamerMode &) = delete;
    StreamerMode &operator=(StreamerMode &&) = delete;

    bool isEnabled() const override;

    void start() override;

private:
    void updated(bool enabled);

    std::unique_ptr<StreamerModePrivate> private_;

    friend class StreamerModePrivate;
};

}  // namespace chatterino
