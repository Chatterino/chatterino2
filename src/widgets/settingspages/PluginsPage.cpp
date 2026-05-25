// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "widgets/settingspages/PluginsPage.hpp"

#    include "Application.hpp"
#    include "common/Args.hpp"
#    include "controllers/accounts/AccountController.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/PluginRepository.hpp"
#    include "controllers/plugins/RemotePlugin.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"
#    include "util/Helpers.hpp"
#    include "util/LayoutCreator.hpp"
#    include "util/RapidJsonSerializeQStringList.hpp"
#    include "util/RemoveScrollAreaBackground.hpp"
#    include "widgets/dialogs/RemotePluginDialog.hpp"
#    include "widgets/helper/EditableModelView.hpp"
#    include "widgets/PluginRepl.hpp"
#    include "widgets/settingspages/SettingWidget.hpp"

#    include <QCheckBox>
#    include <QDialogButtonBox>
#    include <QFormLayout>
#    include <QGroupBox>
#    include <QHeaderView>
#    include <QInputDialog>
#    include <QLabel>
#    include <QListView>
#    include <QMessageBox>
#    include <QObject>
#    include <QPushButton>
#    include <QStringListModel>
#    include <QTableView>
#    include <QWidget>

using namespace Qt::Literals;

namespace {

using namespace chatterino;

class EditRepositoriesDialog : public QDialog
{
public:
    EditRepositoriesDialog(QWidget *parent);
};

EditRepositoriesDialog::EditRepositoriesDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(u"Edit Plugin Repositories"_s);

    auto self = LayoutCreator(this).emplace<QVBoxLayout>();

    self.emplace<QLabel>(
            "Add base URLs of plugin repositories. An entry with the "
            "name <tt>(default)</tt> refers to the default Chatterino "
            "plugin repository.")
        ->setWordWrap(true);

    auto *model = new QStringListModel(getSettings()->remotePluginURLs, this);

    // Buttons
    auto buttonLayout = self.emplace<QHBoxLayout>();
    auto addButton = buttonLayout.emplace<QPushButton>("Add");
    auto removeButton = buttonLayout.emplace<QPushButton>("Remove");

    auto *listView = new QListView;
    self->addWidget(listView, 1);
    listView->setModel(model);

    addButton.onClick(this, [this, model] {
        auto url = QInputDialog::getText(this, "Add URL", "URL").trimmed();
        if (url.isEmpty() || model->stringList().contains(url))
        {
            return;
        }
        if (model->insertRow(model->rowCount()))
        {
            QModelIndex index = model->index(model->rowCount() - 1, 0);
            model->setData(index, url);
        }
    });
    removeButton.onClick(this, [listView, model] {
        auto index = listView->selectionModel()->currentIndex();
        model->removeRow(index.row());
    });

    self.emplace<QDialogButtonBox>(QDialogButtonBox::Ok |
                                   QDialogButtonBox::Cancel)
        .connect(&QDialogButtonBox::accepted, this,
                 [this, model] {
                     getSettings()->remotePluginURLs = model->stringList();
                     this->close();
                 })
        .connect(&QDialogButtonBox::rejected, this,
                 &EditRepositoriesDialog::close);
}

}  // namespace

namespace chatterino {

class PluginsPage::RemoteTableModel : public QAbstractTableModel
{
public:
    using Row =
        std::variant<RemotePluginPtr, std::shared_ptr<PluginRepository>>;

    enum class Column : uint8_t {
        Title,
        Version,
    };
    static constexpr int COLUMN_COUNT = magic_enum::enum_count<Column>();

    static constexpr int PLUGIN_ROLE = Qt::UserRole + 1;

    RemoteTableModel(const QFont &tableFont, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , titleFont(tableFont)
    {
        this->titleFont.setPointSize(16);
    }

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex & /*parent*/) const override
    {
        return COLUMN_COUNT;
    }
    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    void appendRepository(PluginRepository &repo);

    void clear();

private:
    QVariant rowData(const RemotePluginPtr &plugin, Column col, int role) const;
    QVariant rowData(const std::shared_ptr<PluginRepository> &repo, Column col,
                     int role) const;

    QFont titleFont;
    std::vector<Row> rows;
};

int PluginsPage::RemoteTableModel::rowCount(
    const QModelIndex & /*parent*/) const
{
    return static_cast<int>(this->rows.size());
}

QVariant PluginsPage::RemoteTableModel::data(const QModelIndex &index,
                                             int role) const
{
    if (index.row() < 0 || index.column() >= COLUMN_COUNT)
    {
        return {};
    }
    auto rowIndex = static_cast<size_t>(index.row());
    if (rowIndex >= this->rows.size())
    {
        return {};
    }
    const auto &row = this->rows[rowIndex];
    auto column = static_cast<Column>(index.column());
    return std::visit(
        [&](const auto &it) {
            return this->rowData(it, column, role);
        },
        row);
}

QVariant PluginsPage::RemoteTableModel::headerData(
    int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole || section < 0 || section >= COLUMN_COUNT)
    {
        return {};
    }
    switch (static_cast<Column>(section))
    {
        case Column::Title:
            return u"Name"_s;
        case Column::Version:
            return u"Version"_s;
    }
    return {};
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QVariant PluginsPage::RemoteTableModel::rowData(const RemotePluginPtr &plugin,
                                                Column col, int role) const
{
    if (role == PLUGIN_ROLE)
    {
        return QVariant::fromValue(plugin);
    }

    switch (col)
    {
        case Column::Title:
            if (role == Qt::DisplayRole)
            {
                return plugin->meta.name;
            }
            break;
        case Column::Version:
            if (role == Qt::DisplayRole)
            {
                return QString::fromStdString(plugin->meta.version.to_string());
            }
            break;
    }
    return {};
}

QVariant PluginsPage::RemoteTableModel::rowData(
    const std::shared_ptr<PluginRepository> &repo, Column col, int role) const
{
    switch (col)
    {
        case Column::Title:
            switch (role)
            {
                case Qt::DisplayRole:
                    return repo->getName();
                case Qt::FontRole:
                    return this->titleFont;
                default:
                    break;
            }
            break;
        case Column::Version:
            break;
    }
    return {};
}

void PluginsPage::RemoteTableModel::appendRepository(PluginRepository &repo)
{
    const int beginRow = static_cast<int>(this->rows.size());
    const int endRow = beginRow + static_cast<int>(repo.getPlugins().size());
    Q_EMIT this->beginInsertRows({}, beginRow, endRow);

    this->rows.emplace_back(repo.shared_from_this());
    for (const auto &plugin : repo.getPlugins())
    {
        this->rows.emplace_back(plugin);
    }

    Q_EMIT this->endInsertRows();
}

void PluginsPage::RemoteTableModel::clear()
{
    if (this->rows.empty())
    {
        return;
    }

    Q_EMIT this->beginRemoveRows({}, 0,
                                 static_cast<int>(this->rows.size()) - 1);
    this->rows.clear();
    Q_EMIT this->endRemoveRows();
}

PluginsPage::PluginsPage()
    : scrollAreaWidget_(nullptr)
    , dataFrame_(nullptr)
    , remoteTableModel(new RemoteTableModel(this->font(), this))
    , remoteTableView(nullptr)
    , showRemoteWarningsButton(nullptr)
{
    LayoutCreator<PluginsPage> layoutCreator(this);
    auto tabs = layoutCreator.emplace<QTabWidget>();
    auto *generalArea = new QScrollArea;
    generalArea->setObjectName("generalPageScrollContent");
    tabs->addTab(generalArea, "General");

    auto scrollArea = LayoutCreator(generalArea);

    auto widget = scrollArea.emplaceScrollAreaWidget();
    this->scrollAreaWidget_ = widget;
    removeScrollAreaBackground(scrollArea.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();

    {
        auto group = layout.emplace<QGroupBox>("General plugin settings");
        this->generalGroup = group.getElement();
        auto groupLayout = group.setLayoutType<QFormLayout>();
        auto *scaryLabel = new QLabel(
            "Plugins can expand functionality of "
            "Chatterino. They can be made in Lua. This functionality is "
            "still in public alpha stage. Use ONLY the plugins you trust. "
            "The permission system is best effort, always "
            "assume plugins can bypass permissions and can execute "
            "arbitrary code. To see how to create plugins " +
            formatRichNamedLink("https://github.com/Chatterino/chatterino2/"
                                "blob/master/docs/wip-plugins.md",
                                "look at the manual") +
            ".");
        scaryLabel->setWordWrap(true);
        scaryLabel->setOpenExternalLinks(true);
        groupLayout->addRow(scaryLabel);

        auto *description =
            new QLabel("You can load plugins by putting them into " +
                       formatRichNamedLink(
                           "file:///" + getApp()->getPaths().pluginsDirectory,
                           "the Plugins directory") +
                       ". Each one is a new directory.");
        description->setOpenExternalLinks(true);
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        groupLayout->addRow(description);

        auto *box = this->createCheckBox("Enable plugins",
                                         getSettings()->pluginsEnabled);
        QObject::connect(box, &QCheckBox::released, [this]() {
            this->rebuildContent();
        });
        groupLayout->addRow(box);
        if (getApp()->getArgs().safeMode)
        {
            box->setEnabled(false);
            auto *disabledLabel = new QLabel(this);
            disabledLabel->setText("Plugins will not be fully loaded because "
                                   "Chatterino is in safe mode. You can still "
                                   "enable and disable them.");
            groupLayout->addRow(disabledLabel);
        }

        if (getSettings()->pluginRepl.enabled)
        {
            groupLayout->addRow(SettingWidget::fontButton(
                "REPL font", getSettings()->pluginRepl.fontFamily,
                &PluginRepl::currentFont, [](const QFont &font) {
                    getSettings()->pluginRepl.fontFamily = font.family();
                    getSettings()->pluginRepl.fontSize = font.pointSize();
                    getSettings()->pluginRepl.fontStyle = font.styleName();
                }));
        }
    }

    // Remote tab
    auto remoteLayout = tabs.appendTab(new QVBoxLayout, "Remote");

    // Top row
    {
        auto buttonBox = remoteLayout.emplace<QHBoxLayout>().withoutMargin();
        buttonBox.emplace<QPushButton>("Manage Repositories")
            .onClick(this, [this] {
                (new EditRepositoriesDialog(this))->show();
            });
        buttonBox.emplace<QPushButton>("Refresh").onClick(
            this, &PluginsPage::refreshRepositories);
        buttonBox->addStretch();
        buttonBox.emplace<QPushButton>("Show Warnings")
            .assign(&this->showRemoteWarningsButton)
            .onClick(this,
                     [this] {
                         auto *box = new QMessageBox(
                             QMessageBox::Warning, "Remote Warnings",
                             this->remoteWarnings, QMessageBox::Ok, this);
                         box->setAttribute(Qt::WA_DeleteOnClose);
                         box->setTextFormat(Qt::MarkdownText);
                         box->show();
                     })
            ->hide();
    }

    auto tableView = remoteLayout.emplace<QTableView>()
                         .assign(&this->remoteTableView)
                         .connect(&QTableView::doubleClicked, this,
                                  &PluginsPage::tableCellClicked);

    tableView->setModel(this->remoteTableModel);
    tableView->setWordWrap(false);
    tableView->setSelectionMode(QTableView::SingleSelection);
    tableView->setSelectionBehavior(QTableView::SelectRows);
    tableView->setDragDropMode(QTableView::NoDragDrop);
    tableView->verticalHeader()->setVisible(false);
    tableView->horizontalHeader()->setSectionsClickable(false);

    this->rebuildContent();
    this->refreshRepositories();
}

PluginsPage::~PluginsPage() = default;

void PluginsPage::rebuildContent()
{
    if (this->dataFrame_ != nullptr)
    {
        this->dataFrame_->deleteLater();
        this->dataFrame_ = nullptr;
    }
    auto frame = LayoutCreator<QFrame>(new QFrame(this));
    this->dataFrame_ = frame.getElement();
    this->scrollAreaWidget_.append(this->dataFrame_);
    auto layout = frame.setLayoutType<QVBoxLayout>();
    layout->setParent(this->dataFrame_);
    for (const auto &[id, plugin] : getApp()->getPlugins()->plugins())
    {
        auto groupHeaderText =
            QString("%1 (%2, from %3)")
                .arg(plugin->meta.name,
                     QString::fromStdString(plugin->meta.version.to_string()),
                     id);
        auto groupBox = layout.emplace<QGroupBox>(groupHeaderText);
        groupBox->setParent(this->dataFrame_);
        auto pluginEntry = groupBox.setLayoutType<QFormLayout>();
        pluginEntry->setParent(groupBox.getElement());

        if (!plugin->meta.isValid())
        {
            QString errors = "<ul>";
            for (const auto &err : plugin->meta.errors)
            {
                errors += "<li>" + err.toHtmlEscaped() + "</li>";
            }
            errors += "</ul>";

            auto *warningLabel = new QLabel(
                "There were errors while loading metadata for this plugin:" +
                    errors,
                this->dataFrame_);
            warningLabel->setTextFormat(Qt::RichText);
            warningLabel->setStyleSheet("color: #f00");
            pluginEntry->addRow(warningLabel);
        }
        if (!plugin->error().isNull())
        {
            auto *errorLabel =
                new QLabel("There was an error while loading this plugin: " +
                               plugin->error(),
                           this->dataFrame_);
            errorLabel->setStyleSheet("color: #f00");
            errorLabel->setWordWrap(true);
            pluginEntry->addRow(errorLabel);
        }

        auto *description =
            new QLabel(plugin->meta.description, this->dataFrame_);
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        pluginEntry->addRow(description);

        QString authorsTxt;
        for (const auto &author : plugin->meta.authors)
        {
            if (!authorsTxt.isEmpty())
            {
                authorsTxt += ", ";
            }

            authorsTxt += author;
        }
        pluginEntry->addRow("Authors",
                            new QLabel(authorsTxt, this->dataFrame_));

        if (!plugin->meta.homepage.isEmpty())
        {
            auto *homepage = new QLabel(formatRichLink(plugin->meta.homepage),
                                        this->dataFrame_);
            homepage->setOpenExternalLinks(true);
            pluginEntry->addRow("Homepage", homepage);
        }
        pluginEntry->addRow("License",
                            new QLabel(plugin->meta.license, this->dataFrame_));

        QString commandsTxt;
        for (const auto &cmdName : plugin->listRegisteredCommands())
        {
            if (!commandsTxt.isEmpty())
            {
                commandsTxt += ", ";
            }

            commandsTxt += cmdName;
        }
        pluginEntry->addRow("Commands",
                            new QLabel(commandsTxt, this->dataFrame_));
        if (!plugin->meta.permissions.empty())
        {
            QString perms = "<ul>";
            for (const auto &perm : plugin->meta.permissions)
            {
                perms += "<li>" + perm.toHtml() + "</li>";
            }
            perms += "</ul>";

            auto *lbl =
                new QLabel("Required permissions:" + perms, this->dataFrame_);
            lbl->setTextFormat(Qt::RichText);
            pluginEntry->addRow(lbl);
        }

        if (plugin->meta.isValid())
        {
            QString toggleTxt = "Enable";
            if (PluginController::isPluginEnabled(id))
            {
                toggleTxt = "Disable";
            }

            auto *toggleButton = new QPushButton(toggleTxt, this->dataFrame_);
            QObject::connect(
                toggleButton, &QPushButton::pressed, [name = id, this]() {
                    std::vector<QString> val =
                        getSettings()->enabledPlugins.getValue();
                    if (PluginController::isPluginEnabled(name))
                    {
                        val.erase(std::remove(val.begin(), val.end(), name),
                                  val.end());
                    }
                    else
                    {
                        val.push_back(name);
                    }
                    getSettings()->enabledPlugins.setValue(val);
                    getApp()->getPlugins()->reload(name);
                    this->rebuildContent();
                });
            pluginEntry->addRow(toggleButton);
        }

        auto *reloadButton = new QPushButton("Reload", this->dataFrame_);
        QObject::connect(reloadButton, &QPushButton::pressed,
                         [name = id, this]() {
                             getApp()->getPlugins()->reload(name);
                             this->rebuildContent();
                         });
        pluginEntry->addRow(reloadButton);
        if (getApp()->getArgs().safeMode)
        {
            reloadButton->setEnabled(false);
        }

        if (getSettings()->pluginRepl.enabled)
        {
            auto *replButton = new QPushButton("Open REPL", this->dataFrame_);
            QObject::connect(replButton, &QPushButton::clicked, [id]() {
                auto *repl = new PluginRepl(id);
                repl->show();
            });
            pluginEntry->addRow(replButton);
        }
    }
}

void PluginsPage::refreshRepositories()
{
    this->repoConnections.clear();
    this->remoteTableModel->clear();
    this->repositories.clear();
    this->showRemoteWarningsButton->hide();
    for (const auto &url : getSettings()->remotePluginURLs.getValue())
    {
        auto &it = this->repositories.emplace_back(
            std::make_shared<PluginRepository>(url));
        this->repoConnections.emplace_back(
            it->onLoaded.connect([this, weak = it->weak_from_this()] {
                auto repo = weak.lock();
                if (!repo)
                {
                    return;
                }
                this->appendRepository(*repo);
            }));
    }

    for (const auto &repo : this->repositories)
    {
        repo->load();
    }
}

void PluginsPage::appendRepository(PluginRepository &repo)
{
    if (repo.hasErrorOrWarnings())
    {
        if (!this->remoteWarnings.isEmpty())
        {
            this->remoteWarnings.append("\n\n");
        }
        this->remoteWarnings += u"# [**`";
        auto name = repo.getName();
        if (name.isEmpty())
        {
            name = repo.getBaseURL().toEncoded();
        }
        this->remoteWarnings += name;
        this->remoteWarnings += u"`**](";
        this->remoteWarnings += repo.getBaseURL().toEncoded();
        this->remoteWarnings += u")\n\n";

        auto err = repo.getError();
        if (!err.isEmpty())
        {
            this->remoteWarnings += err;
        }
        auto warnings = repo.getWarnings();
        if (!warnings.isEmpty())
        {
            this->remoteWarnings += warnings;
        }

        if (!this->showRemoteWarningsButton->isVisible())
        {
            this->showRemoteWarningsButton->show();
        }
        return;
    }

    this->remoteTableModel->appendRepository(repo);
    QTimer::singleShot(1, this, [this] {
        this->remoteTableView->resizeColumnsToContents();
        this->remoteTableView->horizontalHeader()->setSectionResizeMode(
            QHeaderView::Interactive);
        this->remoteTableView->horizontalHeader()->setSectionResizeMode(
            1, QHeaderView::Stretch);
    });
}

void PluginsPage::tableCellClicked(const QModelIndex &index)
{
    auto ptr = qvariant_cast<RemotePluginPtr>(
        index.data(RemoteTableModel::PLUGIN_ROLE));
    if (!ptr)
    {
        return;
    }

    (new RemotePluginDialog(ptr, this))->show();
}

}  // namespace chatterino

#endif
