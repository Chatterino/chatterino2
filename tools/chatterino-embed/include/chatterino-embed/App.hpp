#pragma once

#include <chatterino-embed/Config.hpp>
#include <QObject>

#include <memory>

class QWidget;
class QString;

namespace chatterino::embed {

class AppPrivate;
class CHATTERINO_EMBED_EXPORT App : public QObject
{
    Q_OBJECT
public:
    ~App() override;

    /// Tell the app that Qt is about to quit.
    ///
    /// This stops services and saves if necessary.
    void aboutToQuit();

protected:
    App(AppPrivate *private_, QObject *parent = nullptr);

private:
    std::unique_ptr<AppPrivate> private_;

    friend AppPrivate;
};

}  // namespace chatterino::embed
