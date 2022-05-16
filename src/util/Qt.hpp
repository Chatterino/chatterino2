#pragma once

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt {
const QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
}
#endif
