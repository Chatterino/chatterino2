#pragma once

#include <pajlada/signals/scoped-connection.hpp>
#include <QSpacerItem>

#include <memory>
#include <vector>

namespace chatterino {

/// A QSpacerItem that scales with Chatterino's scale
class ScalingSpacerItem : public QSpacerItem
{
public:
    static ScalingSpacerItem *horizontal(int baseWidth);
    static ScalingSpacerItem *vertical(int baseHeight);

private:
    ScalingSpacerItem(QSize baseSize, QSizePolicy::Policy horiz,
                      QSizePolicy::Policy vert);

    void refresh();

    QSize baseSize;
    QSize scaledSize;

    QSizePolicy::Policy horizontalPolicy;
    QSizePolicy::Policy verticalPolicy;

    std::vector<std::unique_ptr<pajlada::Signals::ScopedConnection>>
        connections;
};

}  // namespace chatterino
