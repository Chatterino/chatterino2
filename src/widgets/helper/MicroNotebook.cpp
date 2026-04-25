// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/MicroNotebook.hpp"

#include "singletons/Theme.hpp"
#include "widgets/buttons/LabelButton.hpp"

#include <QFrame>

namespace {

using namespace chatterino;

QWidget *makeLine(bool horizontal)
{
    auto *line = new QFrame;
    line->setFrameShape(horizontal ? QFrame::HLine : QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    return line;
}

QWidget *makeHLine()
{
    return makeLine(true);
}
QWidget *makeVLine()
{
    return makeLine(false);
}

class MicroNotebookButton : public LabelButton
{
public:
    MicroNotebookButton(const QString &text)
        : LabelButton(text, nullptr, {6, 3})
    {
    }

    void setSelected(bool selected)
    {
        if (this->selected == selected)
        {
            return;
        }
        this->selected = selected;
        this->invalidateContent();
    }

protected:
    void paintContent(QPainter &painter) override
    {
        painter.setPen({});
        if (this->selected)
        {
            painter.setBrush(getTheme()->tabs.selected.backgrounds.regular);
        }
        else
        {
            painter.setBrush(getTheme()->tabs.regular.backgrounds.regular);
        }
        painter.drawRect(this->rect());

        if (this->selected)
        {
            painter.setPen({getTheme()->tabs.selected.line.regular, 1});
            painter.drawLine(this->rect().topLeft() + QPoint{0, 1},
                             this->rect().topRight() + QPoint{0, 1});
        }
    }

private:
    bool selected = false;
};
}  // namespace

namespace chatterino {

MicroNotebook::MicroNotebook(QWidget *parent)
    : QWidget(parent)
    , topWidget(new QWidget)
    , horizontalSeparator(makeHLine())
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(0);

    this->topWidget->setLayout(&this->topBar);
    rootLayout->addWidget(this->topWidget);
    rootLayout->addWidget(this->horizontalSeparator);

    auto *pageWidget = new QWidget;
    pageWidget->setLayout(&this->layout);
    rootLayout->addWidget(pageWidget, 1);

    rootLayout->setContentsMargins({});
    this->layout.setContentsMargins({});
    this->topBar.setContentsMargins({10, 10, 10, 0});
}

int MicroNotebook::addPage(QWidget *page, QString name)
{
    int index = this->layout.addWidget(page);
    if (!this->items.empty())
    {
        this->topBar.addSpacing(1);
        this->topBar.addWidget(makeVLine());
        this->topBar.addSpacing(1);
    }
    auto *pageButton = new MicroNotebookButton(name);
    this->topBar.addWidget(pageButton);
    QObject::connect(pageButton, &MicroNotebookButton::leftClicked, this,
                     [this, index] {
                         this->layout.setCurrentIndex(index);
                     });
    QObject::connect(&this->layout, &QStackedLayout::currentChanged, pageButton,
                     [pageButton, index](int currentIndex) {
                         pageButton->setSelected(index == currentIndex);
                     });
    pageButton->setSelected(this->layout.currentIndex() == index);

    this->items.emplace_back(Item{.name = std::move(name), .index = index});
    return index;
}

void MicroNotebook::select(QWidget *page)
{
    this->layout.setCurrentWidget(page);
}

bool MicroNotebook::isSelected(QWidget *page) const
{
    return this->layout.currentWidget() == page;
}

void MicroNotebook::setShowHeader(bool showHeader)
{
    this->topWidget->setVisible(showHeader);
    this->horizontalSeparator->setVisible(showHeader);
}

}  // namespace chatterino
