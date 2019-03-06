#pragma once

#include <QVBoxLayout>

namespace ab
{
    /// QVBoxLayout without spacing for content margins.
    class Column : public QVBoxLayout
    {
        Q_OBJECT

    public:
        Column();
    };
}  // namespace ab
