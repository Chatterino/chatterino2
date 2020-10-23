#include "GeneralPageView.hpp"
#include <QScrollBar>
#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutHelper.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/ColorButton.hpp"
#include "widgets/helper/Line.hpp"

namespace chatterino {

GeneralPageView::GeneralPageView(QWidget *parent)
    : QWidget(parent)
{
    auto scrollArea = this->contentScrollArea_ =
        makeScrollArea(this->contentLayout_ = new QVBoxLayout);
    scrollArea->setObjectName("generalSettingsScrollContent");

    auto navigation =
        wrapLayout(this->navigationLayout_ = makeLayout<QVBoxLayout>({}));
    navigation->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    this->navigationLayout_->setAlignment(Qt::AlignTop);
    this->navigationLayout_->addSpacing(6);

    this->setLayout(makeLayout<QHBoxLayout>(
        {scrollArea, new QSpacerItem(16, 1), navigation}));

    QObject::connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
                     this, [=] { this->updateNavigationHighlighting(); });
}

void GeneralPageView::addWidget(QWidget *widget)
{
    this->contentLayout_->addWidget(widget);
}

void GeneralPageView::addLayout(QLayout *layout)
{
    this->contentLayout_->addLayout(layout);
}

TitleLabel *GeneralPageView::addTitle(const QString &title)
{
    // space
    if (!this->groups_.empty())
        this->addWidget(this->groups_.back().space = new Space);

    // title
    auto label = new TitleLabel(title + ":");
    this->addWidget(label);

    // navigation item
    auto navLabel = new NavigationLabel(title);
    navLabel->setCursor(Qt::PointingHandCursor);
    this->navigationLayout_->addWidget(navLabel);

    QObject::connect(navLabel, &NavigationLabel::leftMouseUp, label, [=] {
        this->contentScrollArea_->verticalScrollBar()->setValue(label->y());
    });

    // groups
    this->groups_.push_back(Group{title, label, navLabel, nullptr, {}});

    if (this->groups_.size() == 1)
        this->updateNavigationHighlighting();

    return label;
}

QCheckBox *GeneralPageView::addCheckbox(const QString &text,
                                        BoolSetting &setting, bool inverse)
{
    auto check = new QCheckBox(text);

    // update when setting changes
    setting.connect(
        [inverse, check](const bool &value, auto) {
            check->setChecked(inverse ^ value);
        },
        this->managedConnections_);

    // update setting on toggle
    QObject::connect(
        check, &QCheckBox::toggled, this,
        [&setting, inverse](bool state) { setting = inverse ^ state; });

    this->addWidget(check);

    // groups
    this->groups_.back().widgets.push_back({check, {text}});

    return check;
}

ComboBox *GeneralPageView::addDropdown(const QString &text,
                                       const QStringList &list)
{
    auto layout = new QHBoxLayout;
    auto combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);
    combo->addItems(list);

    auto label = new QLabel(text + ":");
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(combo);

    this->addLayout(layout);

    // groups
    this->groups_.back().widgets.push_back({combo, {text}});
    this->groups_.back().widgets.push_back({label, {text}});

    return combo;
}

ComboBox *GeneralPageView::addDropdown(
    const QString &text, const QStringList &items,
    pajlada::Settings::Setting<QString> &setting, bool editable)
{
    auto combo = this->addDropdown(text, items);

    if (editable)
        combo->setEditable(true);

    // update when setting changes
    setting.connect(
        [combo](const QString &value, auto) { combo->setCurrentText(value); },
        this->managedConnections_);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [&setting](const QString &newValue) {
                         setting = newValue;
                         getApp()->windows->forceLayoutChannelViews();
                     });

    return combo;
}

ColorButton *GeneralPageView::addColorButton(
    const QString &text, const QColor &color,
    pajlada::Settings::Setting<QString> &setting)
{
    auto colorButton = new ColorButton(color);
    auto layout = new QHBoxLayout();
    auto label = new QLabel(text + ":");
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(colorButton);
    this->addLayout(layout);
    QObject::connect(
        colorButton, &ColorButton::clicked, [&setting, colorButton]() {
            auto dialog = new ColorPickerDialog(QColor(setting));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->closed.connect([&setting, colorButton, &dialog] {
                QColor selected = dialog->selectedColor();

                if (selected.isValid())
                {
                    setting = selected.name(QColor::HexArgb);
                    colorButton->setColor(selected);
                }
            });
        });

    this->groups_.back().widgets.push_back({label, {text}});
    this->groups_.back().widgets.push_back({colorButton, {text}});

    return colorButton;
}

void GeneralPageView::addNavigationSpacing()
{
    this->navigationLayout_->addSpacing(24);
}

DescriptionLabel *GeneralPageView::addDescription(const QString &text)
{
    auto label = new DescriptionLabel(text);

    label->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                   Qt::LinksAccessibleByKeyboard);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);

    this->addWidget(label);

    // groups
    this->groups_.back().widgets.push_back({label, {text}});

    return label;
}

void GeneralPageView::addSeperator()
{
    this->addWidget(new Line(false));
}

bool GeneralPageView::filterElements(const QString &query)
{
    bool any{};

    for (auto &&group : this->groups_)
    {
        // if a description in a group matches `query` then show the entire group
        bool descriptionMatches{};
        for (auto &&widget : group.widgets)
        {
            if (auto x = dynamic_cast<DescriptionLabel *>(widget.element); x)
            {
                if (x->text().contains(query, Qt::CaseInsensitive))
                {
                    descriptionMatches = true;
                    break;
                }
            }
        }

        // if group name matches then all should be visible
        if (group.name.contains(query, Qt::CaseInsensitive) ||
            descriptionMatches)
        {
            for (auto &&widget : group.widgets)
                widget.element->show();
            group.title->show();
            any = true;
        }
        // check if any match
        else
        {
            auto groupAny = false;

            for (auto &&widget : group.widgets)
            {
                for (auto &&keyword : widget.keywords)
                {
                    if (keyword.contains(query, Qt::CaseInsensitive))
                    {
                        widget.element->show();
                        groupAny = true;
                    }
                    else
                    {
                        widget.element->hide();
                    }
                }
            }

            if (group.space)
                group.space->setVisible(groupAny);
            group.title->setVisible(groupAny);
            any |= groupAny;
        }
    }

    return any;
}

void GeneralPageView::updateNavigationHighlighting()
{
    auto scrollY = this->contentScrollArea_->verticalScrollBar()->value();
    auto first = true;

    for (auto &&group : this->groups_)
    {
        if (first && (group.title->geometry().bottom() > scrollY ||
                      &group == &this->groups_.back()))
        {
            first = false;
            group.navigationLink->setStyleSheet("color: #00ABF4");
        }
        else
        {
            group.navigationLink->setStyleSheet("");
        }
    }
}

}  // namespace chatterino
