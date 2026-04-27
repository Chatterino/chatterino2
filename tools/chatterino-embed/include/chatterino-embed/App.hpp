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

protected:
    App(AppPrivate *private_, QObject *parent = nullptr);

private:
    std::unique_ptr<AppPrivate> private_;

    friend AppPrivate;
};

}  // namespace chatterino::embed
