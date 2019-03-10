#include "ab/util/MakeWidget.hpp"

#include <QBoxLayout>

namespace ab
{
    void addStretch(void*)
    {
        assert(false);
    }

    void addStretch(QBoxLayout* box)
    {
        box->addStretch();
    }

    QObject* stretch()
    {
        static QObject obj;
        return &obj;
    }
}  // namespace ab
