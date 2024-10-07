#include "controllers/hotkeys/HotkeyController.hpp"

#include "common/QLogging.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "singletons/Settings.hpp"

#include <QShortcut>

namespace {

using namespace chatterino;

const std::map<HotkeyCategory, HotkeyCategoryData> HOTKEY_CATEGORIES = {
    {HotkeyCategory::PopupWindow, {"popupWindow", "Popup Windows"}},
    {HotkeyCategory::Split, {"split", "Split"}},
    {HotkeyCategory::SplitInput, {"splitInput", "Split input box"}},
    {HotkeyCategory::Window, {"window", "Window"}},
};

}  // namespace

namespace chatterino {

const std::map<HotkeyCategory, HotkeyCategoryData> &hotkeyCategories()
{
    return HOTKEY_CATEGORIES;
}

QString hotkeyCategoryName(HotkeyCategory category)
{
    if (!HOTKEY_CATEGORIES.contains(category))
    {
        qCWarning(chatterinoHotkeys) << "Invalid HotkeyCategory passed to "
                                        "categoryDisplayName function";
        return {};
    }

    const auto &categoryData = HOTKEY_CATEGORIES.at(category);

    return categoryData.name;
}

QString hotkeyCategoryDisplayName(HotkeyCategory category)
{
    if (!HOTKEY_CATEGORIES.contains(category))
    {
        qCWarning(chatterinoHotkeys) << "Invalid HotkeyCategory passed to "
                                        "categoryDisplayName function";
        return {};
    }

    const auto &categoryData = HOTKEY_CATEGORIES.at(category);

    return categoryData.displayName;
}

static bool hotkeySortCompare_(const std::shared_ptr<Hotkey> &a,
                               const std::shared_ptr<Hotkey> &b)
{
    if (a->category() == b->category())
    {
        return a->name() < b->name();
    }

    return a->category() < b->category();
}

HotkeyController::HotkeyController()
    : hotkeys_(hotkeySortCompare_)
{
    this->loadHotkeys();

    this->clearRemovedDefaults();

    this->signalHolder_.managedConnect(
        this->hotkeys_.delayedItemsChanged, [this]() {
            qCDebug(chatterinoHotkeys) << "Reloading hotkeys!";
            this->onItemsUpdated.invoke();
        });
}

HotkeyModel *HotkeyController::createModel(QObject *parent)
{
    HotkeyModel *model = new HotkeyModel(parent);
    model->initialize(&this->hotkeys_);
    return model;
}

std::vector<QShortcut *> HotkeyController::shortcutsForCategory(
    HotkeyCategory category,
    std::map<QString, std::function<QString(std::vector<QString>)>> actionMap,
    QWidget *parent)
{
    std::vector<QShortcut *> output;
    for (const auto &hotkey : this->hotkeys_)
    {
        if (hotkey->category() != category)
        {
            continue;
        }
        auto target = actionMap.find(hotkey->action());
        if (target == actionMap.end())
        {
            qCDebug(chatterinoHotkeys)
                << qPrintable(parent->objectName())
                << "Unimplemented hotkey action:" << hotkey->action() << "in"
                << hotkey->getCategory();
            continue;
        }
        if (!target->second)
        {
            // Widget has chosen to explicitly not handle this action
            continue;
        }
        auto createShortcutFromKeySeq = [&](QKeySequence qs) {
            auto *s = new QShortcut(qs, parent);
            s->setContext(hotkey->getContext());
            auto functionPointer = target->second;
            QObject::connect(s, &QShortcut::activated, parent,
                             [functionPointer, hotkey, this]() {
                                 QString output =
                                     functionPointer(hotkey->arguments());
                                 if (!output.isEmpty())
                                 {
                                     this->showHotkeyError(hotkey, output);
                                 }
                             });
            output.push_back(s);
        };
        auto qs = QKeySequence(hotkey->keySequence());

        auto stringified = qs.toString(QKeySequence::NativeText);
        if (stringified.contains("Return"))
        {
            stringified.replace("Return", "Enter");
            auto copy = QKeySequence(stringified, QKeySequence::NativeText);
            createShortcutFromKeySeq(copy);
        }
        createShortcutFromKeySeq(qs);
    }
    return output;
}

void HotkeyController::save()
{
    this->saveHotkeys();
}

std::shared_ptr<Hotkey> HotkeyController::getHotkeyByName(QString name)
{
    for (const auto &hotkey : this->hotkeys_)
    {
        if (hotkey->name() == name)
        {
            return hotkey;
        }
    }
    return nullptr;
}

int HotkeyController::replaceHotkey(QString oldName,
                                    std::shared_ptr<Hotkey> newHotkey)
{
    int i = 0;
    for (const auto &hotkey : this->hotkeys_)
    {
        if (hotkey->name() == oldName)
        {
            this->hotkeys_.removeAt(i);
            break;
        }
        i++;
    }
    return this->hotkeys_.append(newHotkey);
}

std::optional<HotkeyCategory> HotkeyController::hotkeyCategoryFromName(
    QString categoryName)
{
    for (const auto &[category, data] : HOTKEY_CATEGORIES)
    {
        if (data.name == categoryName)
        {
            return category;
        }
    }
    qCDebug(chatterinoHotkeys) << "Unknown category: " << categoryName;
    return {};
}

bool HotkeyController::isDuplicate(std::shared_ptr<Hotkey> hotkey,
                                   QString ignoreNamed)
{
    for (const auto &shared : this->hotkeys_)
    {
        if (shared->name() == ignoreNamed || shared->name() == hotkey->name())
        {
            // Given hotkey is the same as shared, just before it was being edited.
            continue;
        }

        if (shared->category() == hotkey->category() &&
            shared->keySequence() == hotkey->keySequence())
        {
            return true;
        }
    }
    return false;
}

const std::set<QString> &HotkeyController::removedOrDeprecatedHotkeys() const
{
    return this->removedOrDeprecatedHotkeys_;
}

void HotkeyController::loadHotkeys()
{
    auto defaultHotkeysAdded =
        pajlada::Settings::Setting<std::vector<QString>>::get(
            "/hotkeys/addedDefaults");
    auto set = std::set<QString>(defaultHotkeysAdded.begin(),
                                 defaultHotkeysAdded.end());

    // set is currently "defaults added in settings"
    auto numDefaultsFromSettings = set.size();

    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/hotkeys");
    this->addDefaults(set);

    // set is currently "all defaults (defaults defined from application + defaults defined in settings)"
    auto numCombinedDefaults = set.size();

    pajlada::Settings::Setting<std::vector<QString>>::set(
        "/hotkeys/addedDefaults", std::vector<QString>(set.begin(), set.end()));

    qCDebug(chatterinoHotkeys) << "Loading hotkeys...";
    for (const auto &key : keys)
    {
        if (key == "addedDefaults")
        {
            continue;
        }

        auto section = "/hotkeys/" + key;
        auto categoryName =
            pajlada::Settings::Setting<QString>::get(section + "/category");
        auto keySequence =
            pajlada::Settings::Setting<QString>::get(section + "/keySequence");
        auto action =
            pajlada::Settings::Setting<QString>::get(section + "/action");
        auto arguments = pajlada::Settings::Setting<std::vector<QString>>::get(
            section + "/arguments");
        qCDebug(chatterinoHotkeys)
            << "Hotkey " << categoryName << keySequence << action << arguments;

        if (categoryName.isEmpty() || keySequence.isEmpty() || action.isEmpty())
        {
            continue;
        }
        auto category = this->hotkeyCategoryFromName(categoryName);
        if (!category)
        {
            continue;
        }
        this->hotkeys_.append(std::make_shared<Hotkey>(
            *category, QKeySequence(keySequence), action, arguments,
            QString::fromStdString(key)));
    }

    if (numDefaultsFromSettings != numCombinedDefaults)
    {
        // some default that the user was not aware of has been added to the application, force a save to ensure shared state between hotkey controller and settings
        this->saveHotkeys();
    }
}

void HotkeyController::saveHotkeys()
{
    auto defaultHotkeysAdded =
        pajlada::Settings::Setting<std::vector<QString>>::get(
            "/hotkeys/addedDefaults");

    // make sure that hotkeys are deleted
    pajlada::Settings::SettingManager::getInstance()->set(
        "/hotkeys", rapidjson::Value(rapidjson::kObjectType));

    // re-add /hotkeys/addedDefaults as previous set call deleted that key
    pajlada::Settings::Setting<std::vector<QString>>::set(
        "/hotkeys/addedDefaults",
        std::vector<QString>(defaultHotkeysAdded.begin(),
                             defaultHotkeysAdded.end()));

    for (const auto &hotkey : this->hotkeys_)
    {
        auto section = "/hotkeys/" + hotkey->name().toStdString();
        pajlada::Settings::Setting<QString>::set(section + "/action",
                                                 hotkey->action());
        pajlada::Settings::Setting<QString>::set(
            section + "/keySequence", hotkey->keySequence().toString());

        auto categoryName = hotkeyCategoryName(hotkey->category());
        pajlada::Settings::Setting<QString>::set(section + "/category",
                                                 categoryName);
        pajlada::Settings::Setting<std::vector<QString>>::set(
            section + "/arguments", hotkey->arguments());
    }
}

void HotkeyController::addDefaults(std::set<QString> &addedHotkeys)
{
    // popup window
    {
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Escape"), "delete",
                            std::vector<QString>(), "close popup window");
        for (int i = 0; i < 8; i++)
        {
            this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                                QKeySequence(QString("Ctrl+%1").arg(i + 1)),
                                "openTab", {QString::number(i)},
                                QString("popup select tab #%1").arg(i + 1));
        }
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Ctrl+9"), "openTab", {"last"},
                            "popup select last tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Ctrl+Tab"), "openTab", {"next"},
                            "popup select next tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Ctrl+Shift+Tab"), "openTab",
                            {"previous"}, "popup select previous tab");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("PgUp"), "scrollPage", {"up"},
                            "popup scroll up");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("PgDown"), "scrollPage", {"down"},
                            "popup scroll down");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Return"), "accept",
                            std::vector<QString>(), "popup accept");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Escape"), "reject",
                            std::vector<QString>(), "popup reject");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::PopupWindow,
                            QKeySequence("Ctrl+F"), "search",
                            std::vector<QString>(), "popup focus search box");
    }

    // split
    {
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+W"), "delete",
                            std::vector<QString>(), "delete");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+R"), "changeChannel",
                            std::vector<QString>(), "change channel");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+F"), "showSearch",
                            std::vector<QString>(), "show search");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+Shift+F"), "showGlobalSearch",
                            std::vector<QString>(), "show global search");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+F5"), "reconnect",
                            std::vector<QString>(), "reconnect");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("F5"), "reloadEmotes",
                            std::vector<QString>(), "reload emotes");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Alt+x"), "createClip",
                            std::vector<QString>(), "create clip");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Alt+left"), "focus", {"left"},
                            "focus left");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Alt+down"), "focus", {"down"},
                            "focus down");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Alt+up"), "focus", {"up"},
                            "focus up");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Alt+right"), "focus", {"right"},
                            "focus right");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("PgUp"), "scrollPage", {"up"},
                            "scroll page up");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("PgDown"), "scrollPage", {"down"},
                            "scroll page down");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+End"), "scrollToBottom",
                            std::vector<QString>(), "scroll to bottom");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+Home"), "scrollToTop",
                            std::vector<QString>(), "scroll to top");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("F10"), "debug",
                            std::vector<QString>(), "open debug popup");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Split,
                            QKeySequence("Ctrl+Alt+N"), "popupOverlay", {},
                            "open overlay");
        this->tryAddDefault(
            addedHotkeys, HotkeyCategory::Split, QKeySequence("Ctrl+Shift+U"),
            "toggleOverlayInertia", {"all"}, "toggle overlay click-through");
    }

    // split input
    {
        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Ctrl+E"), "openEmotesPopup",
                            std::vector<QString>(), "emote picker");

        // all variations of send message :)
        {
            this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                                QKeySequence("Return"), "sendMessage",
                                std::vector<QString>(), "send message");
            this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                                QKeySequence("Ctrl+Return"), "sendMessage",
                                {"keepInput"}, "send message and keep text");

            this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                                QKeySequence("Shift+Return"), "sendMessage",
                                std::vector<QString>(), "send message");
            this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                                QKeySequence("Ctrl+Shift+Return"),
                                "sendMessage", {"keepInput"},
                                "send message and keep text");
        }

        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Home"), "cursorToStart",
                            {"withoutSelection"}, "go to start of input");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("End"), "cursorToEnd",
                            {"withoutSelection"}, "go to end of input");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Shift+Home"), "cursorToStart",
                            {"withSelection"},
                            "go to start of input with selection");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Shift+End"), "cursorToEnd",
                            {"withSelection"},
                            "go to end of input with selection");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Up"), "previousMessage",
                            std::vector<QString>(), "previous message");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::SplitInput,
                            QKeySequence("Down"), "nextMessage",
                            std::vector<QString>(), "next message");
    }

    // window
    {
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+P"), "openSettings",
                            std::vector<QString>(), "open settings");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+T"), "newSplit",
                            std::vector<QString>(), "new split");
        for (int i = 0; i < 8; i++)
        {
            this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                                QKeySequence(QString("Ctrl+%1").arg(i + 1)),
                                "openTab", {QString::number(i)},
                                QString("select tab #%1").arg(i + 1));
        }
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+9"), "openTab", {"last"},
                            "select last tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+Tab"), "openTab", {"next"},
                            "select next tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+Shift+Tab"), "openTab",
                            {"previous"}, "select previous tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+N"), "popup", {"split"},
                            "new popup window");
        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+Shift+N"), "popup", {"window"},
                            "new popup window from tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence::ZoomIn, "zoom", {"in"}, "zoom in");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence::ZoomOut, "zoom", {"out"}, "zoom out");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("CTRL+0"), "zoom", {"reset"},
                            "zoom reset");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+Shift+T"), "newTab",
                            std::vector<QString>(), "new tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+Shift+W"), "removeTab",
                            std::vector<QString>(), "remove tab");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+G"), "reopenSplit",
                            std::vector<QString>(), "reopen split");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+H"), "toggleLocalR9K",
                            std::vector<QString>(), "toggle local r9k");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+K"), "openQuickSwitcher",
                            std::vector<QString>(), "open quick switcher");

        this->tryAddDefault(addedHotkeys, HotkeyCategory::Window,
                            QKeySequence("Ctrl+U"), "setTabVisibility",
                            {"toggle"}, "toggle tab visibility");
    }
}

void HotkeyController::resetToDefaults()
{
    std::set<QString> addedSet;
    pajlada::Settings::Setting<std::vector<QString>>::set(
        "/hotkeys/addedDefaults",
        std::vector<QString>(addedSet.begin(), addedSet.end()));
    auto size = this->hotkeys_.raw().size();
    for (unsigned long i = 0; i < size; i++)
    {
        this->hotkeys_.removeAt(0);
    }

    // add defaults back
    this->saveHotkeys();
    this->loadHotkeys();
}

void HotkeyController::clearRemovedDefaults()
{
    // The "toggleLiveOnly" argument was removed 2024-08-04
    this->tryRemoveDefault(HotkeyCategory::Window, QKeySequence("Ctrl+Shift+L"),
                           "setTabVisibility", {"toggleLiveOnly"},
                           "toggle live tabs only");

    this->warnForRemovedHotkeyActions(HotkeyCategory::Window,
                                      "setTabVisibility", {"toggleLiveOnly"});
}

void HotkeyController::tryAddDefault(std::set<QString> &addedHotkeys,
                                     HotkeyCategory category,
                                     QKeySequence keySequence, QString action,
                                     std::vector<QString> args, QString name)
{
    qCDebug(chatterinoHotkeys) << "Try add default" << name;
    if (addedHotkeys.count(name) != 0)
    {
        qCDebug(chatterinoHotkeys) << "Already exists";
        return;  // hotkey was added before
    }
    qCDebug(chatterinoHotkeys) << "Inserted";
    this->hotkeys_.append(
        std::make_shared<Hotkey>(category, keySequence, action, args, name));
    addedHotkeys.insert(name);
}

bool HotkeyController::tryRemoveDefault(HotkeyCategory category,
                                        QKeySequence keySequence,
                                        QString action,
                                        std::vector<QString> args, QString name)
{
    return this->hotkeys_.removeFirstMatching([&](const auto &hotkey) {
        return hotkey->category() == category &&
               hotkey->keySequence() == keySequence &&
               hotkey->action() == action && hotkey->arguments() == args &&
               hotkey->name() == name;
    });
}

void HotkeyController::warnForRemovedHotkeyActions(HotkeyCategory category,
                                                   QString action,
                                                   std::vector<QString> args)
{
    for (const auto &hotkey : this->hotkeys_)
    {
        if (hotkey->category() == category && hotkey->action() == action &&
            hotkey->arguments() == args)
        {
            this->removedOrDeprecatedHotkeys_.insert(hotkey->name());
        }
    }
}

void HotkeyController::showHotkeyError(const std::shared_ptr<Hotkey> &hotkey,
                                       QString warning)
{
    auto *msgBox = new QMessageBox(
        QMessageBox::Icon::Warning, "Hotkey error",
        QString(
            "There was an error while executing your hotkey named \"%1\": \n%2")
            .arg(hotkey->name(), warning),
        QMessageBox::Ok);
    msgBox->exec();
}

QKeySequence HotkeyController::getDisplaySequence(
    HotkeyCategory category, const QString &action,
    const std::optional<std::vector<QString>> &arguments) const
{
    const auto &found = this->findLike(category, action, arguments);
    if (found != nullptr)
    {
        return found->keySequence();
    }
    return {};
}

std::shared_ptr<Hotkey> HotkeyController::findLike(
    HotkeyCategory category, const QString &action,
    const std::optional<std::vector<QString>> &arguments) const
{
    for (auto other : this->hotkeys_)
    {
        if (other->category() == category && other->action() == action)
        {
            if (arguments)
            {
                if (other->arguments() == *arguments)
                {
                    return other;
                }
            }
            else
            {
                return other;
            }
        }
    }
    return nullptr;
}

}  // namespace chatterino
