#include "widgets/settingspages/GeneralPageView.hpp"

#include "Application.hpp"
#include "util/LayoutHelper.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorButton.hpp"
#include "widgets/helper/Line.hpp"

#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>

namespace {

constexpr int MAX_TOOLTIP_LINE_LENGTH = 50;
const auto MAX_TOOLTIP_LINE_LENGTH_PATTERN =
    QStringLiteral(R"(.{%1}\S*\K(\s+))").arg(MAX_TOOLTIP_LINE_LENGTH);
const QRegularExpression MAX_TOOLTIP_LINE_LENGTH_REGEX(
    MAX_TOOLTIP_LINE_LENGTH_PATTERN);
}  // namespace

namespace chatterino {

GeneralPageView::GeneralPageView(QWidget *parent)
    : QWidget(parent)
{
    auto *scrollArea = this->contentScrollArea_ =
        makeScrollArea(this->contentLayout_ = new QVBoxLayout);
    scrollArea->setObjectName("generalSettingsScrollContent");

    auto *navigation =
        wrapLayout(this->navigationLayout_ = makeLayout<QVBoxLayout>({}));
    navigation->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    this->navigationLayout_->setAlignment(Qt::AlignTop);
    this->navigationLayout_->addSpacing(6);

    this->setLayout(makeLayout<QHBoxLayout>(
        {scrollArea, new QSpacerItem(16, 1), navigation}));

    QObject::connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
                     this, [this] {
                         this->updateNavigationHighlighting();
                     });
}

void GeneralPageView::addWidget(QWidget *widget)
{
    this->contentLayout_->addWidget(widget);
}

void GeneralPageView::addLayout(QLayout *layout)
{
    this->contentLayout_->addLayout(layout);
}

void GeneralPageView::addStretch()
{
    this->contentLayout_->addStretch();
}

TitleLabel *GeneralPageView::addTitle(const QString &title)
{
    // space
    if (!this->groups_.empty())
    {
        this->addWidget(this->groups_.back().space = new Space);
    }

    // title
    auto *label = new TitleLabel(title + ":");
    this->addWidget(label);

    // navigation item
    auto *navLabel = new NavigationLabel(title);
    navLabel->setCursor(Qt::PointingHandCursor);
    this->navigationLayout_->addWidget(navLabel);

    QObject::connect(navLabel, &NavigationLabel::leftMouseUp, label, [=, this] {
        this->contentScrollArea_->verticalScrollBar()->setValue(label->y());
    });

    // groups
    this->groups_.push_back(Group{title, label, navLabel, nullptr, {}});

    if (this->groups_.size() == 1)
    {
        this->updateNavigationHighlighting();
    }

    return label;
}

SubtitleLabel *GeneralPageView::addSubtitle(const QString &title)
{
    auto *label = new SubtitleLabel(title + ":");
    this->addWidget(label);

    this->groups_.back().widgets.push_back({label, {title}});

    return label;
}

QCheckBox *GeneralPageView::addCheckbox(const QString &text,
                                        BoolSetting &setting, bool inverse,
                                        QString toolTipText)
{
    auto *check = new QCheckBox(text);
    this->addToolTip(*check, toolTipText);

    // update when setting changes
    setting.connect(
        [inverse, check](const bool &value, auto) {
            check->setChecked(inverse ^ value);
        },
        this->managedConnections_);

    // update setting on toggle
    QObject::connect(check, &QCheckBox::toggled, this,
                     [&setting, inverse](bool state) {
                         setting = inverse ^ state;
                     });

    this->addWidget(check);

    // groups
    this->groups_.back().widgets.push_back({check, {text}});

    return check;
}

QCheckBox *GeneralPageView::addCustomCheckbox(const QString &text,
                                              const std::function<bool()> &load,
                                              std::function<void(bool)> save,
                                              const QString &toolTipText)
{
    auto *check = new QCheckBox(text);
    this->addToolTip(*check, toolTipText);

    check->setChecked(load());

    QObject::connect(check, &QCheckBox::toggled, this,
                     [save = std::move(save)](bool state) {
                         save(state);
                     });

    this->addWidget(check);

    this->groups_.back().widgets.push_back({check, {text}});

    return check;
}

ComboBox *GeneralPageView::addDropdown(const QString &text,
                                       const QStringList &list,
                                       QString toolTipText)
{
    auto *layout = new QHBoxLayout;
    auto *combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);
    combo->addItems(list);

    auto *label = new QLabel(text + ":");
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(combo);

    this->addToolTip(*label, toolTipText);
    this->addLayout(layout);

    // groups
    this->groups_.back().widgets.push_back({combo, {text}});
    this->groups_.back().widgets.push_back({label, {text}});

    return combo;
}

ComboBox *GeneralPageView::addDropdown(
    const QString &text, const QStringList &items,
    pajlada::Settings::Setting<QString> &setting, bool editable,
    QString toolTipText)
{
    auto *combo = this->addDropdown(text, items, toolTipText);

    if (editable)
    {
        combo->setEditable(true);
    }

    // update when setting changes
    setting.connect(
        [combo](const QString &value, auto) {
            combo->setCurrentText(value);
        },
        this->managedConnections_);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [&setting](const QString &newValue) {
                         setting = newValue;
                         getApp()->getWindows()->forceLayoutChannelViews();
                     });

    return combo;
}

ColorButton *GeneralPageView::addColorButton(
    const QString &text, const QColor &color,
    pajlada::Settings::Setting<QString> &setting, QString toolTipText)
{
    auto *colorButton = new ColorButton(color);
    auto *layout = new QHBoxLayout();
    auto *label = new QLabel(text + ":");

    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(colorButton);

    this->addToolTip(*label, toolTipText);
    this->addLayout(layout);

    QObject::connect(
        colorButton, &ColorButton::clicked, [this, &setting, colorButton]() {
            auto *dialog = new ColorPickerDialog(QColor(setting), this);
            // colorButton & setting are never deleted and the signal is deleted
            // once the dialog is closed
            QObject::connect(dialog, &ColorPickerDialog::colorConfirmed, this,
                             [&setting, colorButton](auto selected) {
                                 if (selected.isValid())
                                 {
                                     setting = selected.name(QColor::HexArgb);
                                     colorButton->setColor(selected);
                                 }
                             });
            dialog->show();
        });

    this->groups_.back().widgets.push_back({label, {text}});
    this->groups_.back().widgets.push_back({colorButton, {text}});

    return colorButton;
}

QSpinBox *GeneralPageView::addIntInput(const QString &text, IntSetting &setting,
                                       int min, int max, int step,
                                       QString toolTipText)
{
    auto *layout = new QHBoxLayout;

    auto *label = new QLabel(text + ":");
    this->addToolTip(*label, toolTipText);

    auto *input = new QSpinBox;
    input->setMinimum(min);
    input->setMaximum(max);

    // update when setting changes
    setting.connect(
        [input](const int &value, auto) {
            input->setValue(value);
        },
        this->managedConnections_);

    // update setting on value changed
    QObject::connect(input, QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [&setting](int newValue) {
                         setting = newValue;
                     });

    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(input);

    this->addLayout(layout);

    // groups
    this->groups_.back().widgets.push_back({input, {text}});
    this->groups_.back().widgets.push_back({label, {text}});

    return input;
}

void GeneralPageView::addNavigationSpacing()
{
    this->navigationLayout_->addSpacing(24);
}

DescriptionLabel *GeneralPageView::addDescription(const QString &text)
{
    auto *label = new DescriptionLabel(text);

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
            if (auto *x = dynamic_cast<DescriptionLabel *>(widget.element); x)
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
            {
                widget.element->show();
            }

            group.title->show();
            group.navigationLink->show();
            any = true;
        }
        // check if any match
        else
        {
            auto groupAny = false;

            QWidget *currentSubtitle = nullptr;
            bool currentSubtitleVisible = false;
            bool currentSubtitleSearched = false;

            for (auto &&widget : group.widgets)
            {
                if (auto *x = dynamic_cast<SubtitleLabel *>(widget.element))
                {
                    currentSubtitleSearched = false;
                    if (currentSubtitle)
                    {
                        currentSubtitle->setVisible(currentSubtitleVisible);
                    }

                    currentSubtitleVisible = false;
                    currentSubtitle = widget.element;

                    if (x->text().contains(query, Qt::CaseInsensitive))
                    {
                        currentSubtitleSearched = true;
                    }
                    continue;
                }

                for (auto &&keyword : widget.keywords)
                {
                    if (keyword.contains(query, Qt::CaseInsensitive) ||
                        currentSubtitleSearched)
                    {
                        currentSubtitleVisible = true;
                        widget.element->show();
                        groupAny = true;
                        break;
                    }

                    widget.element->hide();
                }
            }

            if (currentSubtitle)
            {
                currentSubtitle->setVisible(currentSubtitleVisible);
            }

            if (group.space)
            {
                group.space->setVisible(groupAny);
            }

            group.title->setVisible(groupAny);
            group.navigationLink->setVisible(groupAny);
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

void GeneralPageView::addToolTip(QWidget &widget, QString text) const
{
    if (text.isEmpty())
    {
        return;
    }

    if (text.length() > MAX_TOOLTIP_LINE_LENGTH)
    {
        // match MAX_TOOLTIP_LINE_LENGTH characters, any remaining
        // non-space, and then capture the following space for
        // replacement with newline
        text.replace(MAX_TOOLTIP_LINE_LENGTH_REGEX, "\n");
    }

    widget.setToolTip(text);
}

}  // namespace chatterino
