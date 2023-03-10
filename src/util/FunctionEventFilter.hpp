#pragma once

#include <QEvent>
#include <QObject>

#include <functional>

namespace chatterino {

class FunctionEventFilter : public QObject
{
    Q_OBJECT

public:
    FunctionEventFilter(QObject *parent,
                        std::function<bool(QObject *, QEvent *)> function);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::function<bool(QObject *, QEvent *)> function_;
};

}  // namespace chatterino
