#pragma once

#include <QHBoxLayout>

namespace ab
{
    /// QHBoxLayout without spacing for content margins.
    class Row : public QHBoxLayout
    {
        Q_OBJECT

    public:
        Row();
    };
}  // namespace ab
