#include "widgets/layout/FlowLayout.hpp"

#include <QSizePolicy>
#include <QStyle>
#include <QtGlobal>
#include <QWidget>

namespace {

using namespace chatterino;

class Linebreak : public QWidget
{
};

}  // namespace

namespace chatterino {

FlowLayout::FlowLayout(QWidget *parent, Options options)
    : QLayout(parent)
    , hSpace_(options.hSpacing)
    , vSpace_(options.vSpacing)
{
    if (options.margin >= 0)
    {
        this->setContentsMargins(options.margin, options.margin, options.margin,
                                 options.margin);
    }
}

FlowLayout::FlowLayout(Options options)
    : FlowLayout(nullptr, options)
{
}

FlowLayout::~FlowLayout()
{
    for (auto *item : this->itemList_)
    {
        delete item;
    }
    this->itemList_ = {};
}

void FlowLayout::addItem(QLayoutItem *item)
{
    this->itemList_.push_back(item);
}

void FlowLayout::addLinebreak(int height)
{
    auto *linebreak = new Linebreak;
    linebreak->setFixedHeight(height);
    this->addWidget(linebreak);
}

int FlowLayout::horizontalSpacing() const
{
    if (this->hSpace_ >= 0)
    {
        return this->hSpace_;
    }

    return this->defaultSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

void FlowLayout::setHorizontalSpacing(int value)
{
    if (this->hSpace_ == value)
    {
        return;
    }
    this->hSpace_ = value;
    this->invalidate();
}

int FlowLayout::verticalSpacing() const
{
    if (this->vSpace_ >= 0)
    {
        return this->vSpace_;
    }

    return this->defaultSpacing(QStyle::PM_LayoutVerticalSpacing);
}

void FlowLayout::setVerticalSpacing(int value)
{
    if (this->vSpace_ == value)
    {
        return;
    }
    this->vSpace_ = value;
    this->invalidate();
}

int FlowLayout::count() const
{
    return static_cast<int>(this->itemList_.size());
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(this->itemList_.size()))
    {
        return this->itemList_[static_cast<size_t>(index)];
    }
    return nullptr;
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < static_cast<int>(this->itemList_.size()))
    {
        auto *it = this->itemList_[static_cast<size_t>(index)];
        this->itemList_.erase(this->itemList_.cbegin() +
                              static_cast<qsizetype>(index));
        return it;
    }
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return this->doLayout({0, 0, width, 0}, true);
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    this->doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return this->minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const auto *item : this->itemList_)
    {
        size = size.expandedTo(item->minimumSize());
    }

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(),
                  margins.top() + margins.bottom());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    auto margins = this->contentsMargins();
    QRect effectiveRect = rect.adjusted(margins.left(), margins.top(),
                                        -margins.right(), -margins.bottom());
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    for (QLayoutItem *item : this->itemList_)
    {
        auto *linebreak = dynamic_cast<Linebreak *>(item->widget());
        if (linebreak)
        {
            item->setGeometry({x, y, 0, linebreak->height()});
            x = effectiveRect.x();
            y = y + lineHeight + linebreak->height();
            lineHeight = 0;
            continue;
        }

        auto space = this->getSpacing(item);
        int nextX = x + item->sizeHint().width() + space.width();
        if (nextX - space.width() > effectiveRect.right() && lineHeight > 0)
        {
            x = effectiveRect.x();
            y = y + lineHeight + space.height();
            nextX = x + item->sizeHint().width() + space.width();
            lineHeight = 0;
        }

        if (!testOnly)
        {
            item->setGeometry({QPoint{x, y}, item->sizeHint()});
        }

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }

    return y + lineHeight - rect.y() + margins.bottom();
}

int FlowLayout::defaultSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent)
    {
        return -1;
    }
    if (auto *widget = dynamic_cast<QWidget *>(parent))
    {
        return widget->style()->pixelMetric(pm, nullptr, widget);
    }
    if (auto *layout = dynamic_cast<QLayout *>(parent))
    {
        return layout->spacing();
    }
    return -1;
}

QSize FlowLayout::getSpacing(QLayoutItem *item) const
{
    // called if there isn't any parent or the parent can't provide any spacing
    auto fallbackSpacing = [&](auto dir) {
        if (auto *widget = item->widget())
        {
            return widget->style()->layoutSpacing(QSizePolicy::PushButton,
                                                  QSizePolicy::PushButton, dir);
        }
        if (auto *layout = item->layout())
        {
            return layout->spacing();
        }
        return 0;
    };

    QSize spacing(this->horizontalSpacing(), this->verticalSpacing());
    if (spacing.width() == -1)
    {
        spacing.rwidth() = fallbackSpacing(Qt::Horizontal);
    }
    if (spacing.height() == -1)
    {
        spacing.rheight() = fallbackSpacing(Qt::Vertical);
    }
    return spacing;
}

}  // namespace chatterino
