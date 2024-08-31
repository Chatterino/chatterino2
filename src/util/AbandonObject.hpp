#pragma once

#include <QObject>

namespace chatterino {

/// Guard to call `deleteLater` on a QObject when destroyed.
class AbandonObject
{
public:
    AbandonObject(QObject *obj)
        : obj_(obj)
    {
    }

    ~AbandonObject()
    {
        if (this->obj_)
        {
            this->obj_->deleteLater();
        }
    }

    AbandonObject(const AbandonObject &) = delete;
    AbandonObject(AbandonObject &&) = delete;
    AbandonObject &operator=(const AbandonObject &) = delete;
    AbandonObject &operator=(AbandonObject &&) = delete;

private:
    QObject *obj_;
};

}  // namespace chatterino
