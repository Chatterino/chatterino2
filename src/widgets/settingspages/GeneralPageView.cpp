#include "widgets/settingspages/GeneralPageView.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "util/LayoutHelper.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "widgets/helper/Line.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

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
    , contentScrollArea_(new QScrollArea)
    , contentLayout_(new QVBoxLayout)
{
    auto *contentWidget = new QWidget;
    contentWidget->setLayout(this->contentLayout_);
    this->contentScrollArea_->setWidget(contentWidget);
    this->contentScrollArea_->setObjectName("generalSettingsScrollContent");
    this->contentScrollArea_->setWidgetResizable(true);
}

GeneralPageView *GeneralPageView::withoutNavigation(QWidget *parent)
{
    auto *view = new GeneralPageView(parent);

    view->setLayout(makeLayout<QHBoxLayout>({
        view->contentScrollArea_,
    }));

    return view;
}

GeneralPageView *GeneralPageView::withNavigation(QWidget *parent)
{
    auto *view = new GeneralPageView(parent);

    auto *navigation =
        wrapLayout(view->navigationLayout_ = makeLayout<QVBoxLayout>({}));
    navigation->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    view->navigationLayout_->setAlignment(Qt::AlignTop);
    view->navigationLayout_->addSpacing(6);

    view->setLayout(makeLayout<QHBoxLayout>({
        view->contentScrollArea_,
        new QSpacerItem(16, 1),
        navigation,
    }));

    QObject::connect(view->contentScrollArea_->verticalScrollBar(),
                     &QScrollBar::valueChanged, view, [view] {
                         view->updateNavigationHighlighting();
                     });

    return view;
}

void GeneralPageView::addWidget(QWidget *widget, const QStringList &keywords)
{
    this->contentLayout_->addWidget(widget);
    if (!this->groups_.empty())
    {
        this->groups_.back().widgets.push_back({
            .element = widget,
            .keywords = keywords,
        });
    }
}

void GeneralPageView::registerWidget(QWidget *widget,
                                     const QStringList &keywords,
                                     QWidget *parentElement)
{
    if (!this->groups_.empty())
    {
        this->groups_.back().widgets.push_back({
            .element = widget,
            .keywords = keywords,
            .parentElement = parentElement,
        });
    }
}

void GeneralPageView::pushWidget(QWidget *widget)
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

    NavigationLabel *navLabel = nullptr;

    // navigation item
    if (this->navigationLayout_ != nullptr)
    {
        navLabel = new NavigationLabel(title);
        navLabel->setCursor(Qt::PointingHandCursor);
        this->navigationLayout_->addWidget(navLabel);

        QObject::connect(
            navLabel, &NavigationLabel::leftMouseUp, label, [this, label] {
                this->contentScrollArea_->verticalScrollBar()->setValue(
                    label->y());
            });
    }

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
    if (inverse)
    {
        qCWarning(chatterinoWidget)
            << "use SettingWidget::inverseCheckbox instead";
    }

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
    assert(this->navigationLayout_ != nullptr &&
           "addNavigationSpacing used without navigation");

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
                if (widget.parentElement != nullptr)
                {
                    widget.parentElement->show();
                }
            }

            group.title->show();
            if (group.navigationLink != nullptr)
            {
                group.navigationLink->show();
            }
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
                        if (widget.parentElement != nullptr)
                        {
                            widget.parentElement->show();
                        }
                        groupAny = true;
                        break;
                    }

                    widget.element->hide();
                    if (widget.parentElement != nullptr)
                    {
                        widget.parentElement->hide();
                    }
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
            if (group.navigationLink != nullptr)
            {
                group.navigationLink->setVisible(groupAny);
            }
            any |= groupAny;
        }
    }

    return any;
}

void GeneralPageView::updateNavigationHighlighting()
{
    if (this->navigationLayout_ == nullptr)
    {
        return;
    }

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
