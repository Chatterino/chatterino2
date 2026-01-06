// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/PluginRepl.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "singletons/Resources.hpp"
#    include "singletons/Settings.hpp"
#    include "singletons/Theme.hpp"
#    include "widgets/buttons/SvgButton.hpp"

#    include <QBoxLayout>
#    include <QFontDatabase>
#    include <QScrollBar>
#    include <QSplitter>
#    include <QTextBlock>
#    include <QTextEdit>
#    include <sol/sol.hpp>

namespace {

using namespace Qt::StringLiterals;
using namespace chatterino;

class HistoricTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    HistoricTextEdit(QWidget *parent = nullptr);

    QSize sizeHint() const override
    {
        return this->minimumSizeHint();
    }

    QSize minimumSizeHint() const override
    {
        auto margins = this->contentsMargins();

        auto h = margins.top() + this->fontMetrics().height() +
                 margins.bottom() + 15;
        return {0, h};
    }

Q_SIGNALS:
    void onSend(const QString & /* text */);

protected:
    bool event(QEvent *e) override;

private:
    /// If the item wouldn't change, std::nullopt is returned
    std::optional<QString> nextHistoryItem(qsizetype diff);

    QStringList history;
    qsizetype historyIdx = 0;
    QString lastUnfinishedInput;
};

HistoricTextEdit::HistoricTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

bool HistoricTextEdit::event(QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
    {
        return QTextEdit::event(event);
    }

    auto *e = dynamic_cast<QKeyEvent *>(event);
    if (!e)
    {
        return false;
    }

    if (e->keyCombination() == QKeyCombination(Qt::Key_Up))
    {
        e->accept();
        auto cursor = this->textCursor();
        if (cursor.movePosition(QTextCursor::Up))
        {
            this->setTextCursor(cursor);
        }
        else
        {
            auto next = this->nextHistoryItem(-1);
            if (next)
            {
                this->setPlainText(*std::move(next));
                this->moveCursor(QTextCursor::End);
            }
        }
        return true;
    }
    if (e->keyCombination() == QKeyCombination(Qt::Key_Down))
    {
        e->accept();
        auto cursor = this->textCursor();
        if (cursor.movePosition(QTextCursor::Down))
        {
            this->setTextCursor(cursor);
        }
        else
        {
            auto next = this->nextHistoryItem(1);
            if (next)
            {
                this->setPlainText(*std::move(next));
                this->moveCursor(QTextCursor::End);
            }
        }
        return true;
    }
    if (e->keyCombination() == QKeyCombination(Qt::Key_Return))
    {
        auto text = this->toPlainText().trimmed();
        if (!text.isEmpty())
        {
            e->accept();
            if (this->history.empty() || this->history.back() != text)
            {
                this->history.append(text);
            }
            this->historyIdx = this->history.size();
            this->setPlainText({});
            this->lastUnfinishedInput = {};
            this->onSend(text);
            return true;
        }
    }

    return QTextEdit::event(event);
}

std::optional<QString> HistoricTextEdit::nextHistoryItem(qsizetype diff)
{
    if (this->history.empty())
    {
        return {};
    }
    bool wasUnfinishedInput = this->historyIdx >= this->history.size();

    auto nextIdx =
        std::clamp<qsizetype>(this->historyIdx + diff, 0, this->history.size());
    if (nextIdx == this->historyIdx)
    {
        return {};  // nothing changed
    }
    this->historyIdx = nextIdx;

    if (this->historyIdx >= this->history.size())
    {
        return this->lastUnfinishedInput;
    }
    if (wasUnfinishedInput)
    {
        this->lastUnfinishedInput = this->toPlainText();
    }
    return this->history[this->historyIdx];
}

sol::optional<QString> tryAsStringAndPop(lua_State *L)
{
    auto s = sol::stack::get<sol::optional<std::string_view>>(L);
    auto qs = s.map([](auto sv) {
        return QString::fromUtf8(sv.data(), sv.size());
    });
    lua_pop(L, 1);
    return qs;
}

/// This stringifies a value without recursing.
///
/// It's similar to luaL_tolstring with a two minor differences:
/// - `__tostring` is not executed - this can potentially error because it might
///   be called without a `self` argument (e.g. in `c2.Channel`).
/// - The address for tables, threads, etc. isn't shown.
QString stringifyValue(lua_State *L, int idx)
{
    switch (lua_type(L, idx))
    {
        case LUA_TNONE:
            return u"(none)"_s;
        case LUA_TNIL:
            return u"nil"_s;
        case LUA_TSTRING: {
            // Safety: We know this is a string
            auto sv = sol::stack::unqualified_get<std::string_view>(L, idx);
            return QString::fromUtf8(sv.data(),
                                     static_cast<qsizetype>(sv.size()));
        }
        case LUA_TNUMBER: {
            if (lua_isinteger(L, idx) != 0)
            {
                auto v = static_cast<LUAI_UACINT>(lua_tointeger(L, idx));
                return QString::number(v);
            }
            auto v = static_cast<LUAI_UACNUMBER>(lua_tonumber(L, idx));
            return QString::number(v);
        }
        case LUA_TBOOLEAN: {
            if (lua_toboolean(L, idx) != 0)
            {
                return u"true"_s;
            }
            return u"false"_s;
        }
        case LUA_TTHREAD:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
        case LUA_TTABLE:
        default: {
            int tt = luaL_getmetafield(L, idx, "__name");
            if (tt != LUA_TNIL)
            {
                auto name = tryAsStringAndPop(L);
                if (name)
                {
                    return *std::move(name);
                }
            }

            return QString::fromUtf8(luaL_typename(L, idx));
        }
    }
}

QString stringifyValue(sol::stack_proxy it)
{
    return stringifyValue(it.lua_state(), it.stack_index());
}

QString stringifyValue(const sol::object &obj)
{
    static_assert(!sol::is_stack_based<std::remove_cvref_t<decltype(obj)>>());
    // XXX: in many cases the value is already on the stack
    obj.push();
    auto s = stringifyValue(obj.lua_state(), -1);
    obj.pop();
    return s;
}

enum class ExpandFlag : uint8_t {
    None = 0,
    TryUsertypeStorage = 1 << 0,
};
using ExpandFlags = FlagsEnum<ExpandFlag>;

void stringify(sol::stack_proxy it, QString &s, size_t maxItems = 10,
               ExpandFlags flags = {})
{
    lua::StackGuard g(it.lua_state());

    auto typ = it.get_type();
    switch (typ)
    {
        case sol::type::userdata: {
            auto meta = lua_getmetatable(it.lua_state(), it.stack_index());
            if (meta == 0)
            {
                s.append(u"userdata");
            }

            auto table =
                sol::stack::unqualified_get<sol::table>(it.lua_state());

            s.append(u"userdata{");
            size_t n = 0;
            for (const auto &inner : table)
            {
                if (n != 0)
                {
                    s.append(u", ");
                }
                s.append(stringifyValue(inner.first));

                n++;
                if (n >= maxItems)
                {
                    s.append(u", ...");
                    break;
                }
            }
            s.append('}');

            lua_pop(it.lua_state(), 1);  // metatable
            return;
        }
        case sol::type::table: {
            auto tbl = it.get<sol::optional<sol::table>>();
            if (!tbl)
            {
                s.append(stringifyValue(it));
                return;
            }
            s.append(u"{");
            size_t n = 0;

            auto meta = lua_getmetatable(it.lua_state(), it.stack_index());
            if (meta != 0)
            {
                s.append(u"(metatable): "_s);
                stringify({it.lua_state(), -1}, s, maxItems / 2,
                          flags | ExpandFlag::TryUsertypeStorage);
                n++;
                lua_pop(it.lua_state(), 1);  // metatable
            }

            if (flags.has(ExpandFlag::TryUsertypeStorage))
            {
                auto uts = sol::u_detail::maybe_get_usertype_storage_base(
                    it.lua_state(), it.stack_index());
                if (uts)
                {
                    for (const auto &[key, _] : uts->string_keys)
                    {
                        if (n == 0)
                        {
                            s.append('[');
                        }
                        else
                        {
                            s.append(u", ["_s);
                        }
#    if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
                        s.append(QString::fromUtf8(
                            key.data(), static_cast<qsizetype>(key.size())));
#    else
                        s.append(QUtf8StringView(key));
#    endif
                        s.append("] = <dyn>");
                        n++;
                    }
                }
            }

            for (const auto &inner : *tbl)
            {
                if (n == 0)
                {
                    s.append('[');
                }
                else
                {
                    s.append(u", ["_s);
                }

                s.append(stringifyValue(inner.first));
                s.append("] = ");
                s.append(stringifyValue(inner.second));

                n++;
                if (n >= maxItems)
                {
                    s.append(u", ..."_s);
                    break;
                }
            }
            s.append('}');
            return;
        }
        case sol::type::none:
        case sol::type::lua_nil:
        case sol::type::string:
        case sol::type::number:
        case sol::type::thread:
        case sol::type::boolean:
        case sol::type::function:
        case sol::type::lightuserdata:
        case sol::type::poly:
        default:
            s.append(stringifyValue(it));
    }
}

}  // namespace

namespace chatterino {

PluginRepl::PluginRepl(QString id, QWidget *parent)
    : BaseWindow(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::DisableCustomScaling,
              BaseWindow::DisableLayoutSave,
          },
          parent)
    , id(std::move(id))
    , isPinned(getSettings()->windowTopMost)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->resize(600, 300);

    this->setWindowTitle(this->id + u" - Plugin REPL"_s);

    auto *root = new QVBoxLayout(this->getLayoutContainer());

    // top row
    {
        auto *top = new QHBoxLayout;
        this->ui.clear = new SvgButton(
            {
                .dark = u":/buttons/cancel.svg"_s,
                .light = u":/buttons/cancelDark.svg"_s,
            },
            nullptr, {3, 3});
        this->ui.clear->setScaleIndependentSize({18, 18});
        this->ui.clear->setToolTip(u"Clear Output"_s);
        QObject::connect(this->ui.clear, &Button::leftClicked, this, [this] {
            this->ui.output->clear();
        });

        this->ui.reload = new SvgButton(
            {
                .dark = u":/buttons/reloadLight.svg"_s,
                .light = u":/buttons/reloadDark.svg"_s,
            },
            nullptr, {3, 3});
        this->ui.reload->setScaleIndependentSize({18, 18});
        this->ui.reload->setToolTip(u"Reload"_s);
        QObject::connect(this->ui.reload, &Button::leftClicked, this, [this] {
            if (!this->plugin)
            {
                this->log(lua::api::LogLevel::Critical, "Plugin not loaded.");
                return;
            }
            this->log({}, u"Reloading..."_s);
            bool result = getApp()->getPlugins()->reload(this->id);
            if (result)
            {
                this->log({}, u"Reloaded."_s);
            }
            else
            {
                this->log(lua::api::LogLevel::Critical, u"Failed to reload."_s);
            }
        });

        this->ui.pin = new SvgButton(this->ui.pinDisabledSource_, this, {3, 3});
        this->ui.pin->setScaleIndependentSize({18, 18});
        this->ui.pin->setToolTip(u"Pin Window"_s);
        QObject::connect(this->ui.pin, &Button::leftClicked, this, [this] {
            this->isPinned = !this->isPinned;
            this->updatePinned();
        });

        top->addStretch(1);
        top->addWidget(this->ui.clear);
        top->addWidget(this->ui.reload);
        top->addWidget(this->ui.pin);

        root->addLayout(top);
    }

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);
    root->addWidget(splitter, 1);

    this->ui.output = new QTextEdit;
    splitter->addWidget(this->ui.output);
    this->ui.output->setUndoRedoEnabled(false);
    this->ui.output->setReadOnly(true);
    this->ui.output->setAcceptRichText(false);
    auto *input = new HistoricTextEdit(this);
    this->ui.input = input;
    QObject::connect(input, &HistoricTextEdit::onSend, this,
                     &PluginRepl::tryRun);
    splitter->addWidget(this->ui.input);
    this->ui.input->setPlaceholderText(u"Type something..."_s);
    this->ui.input->setAcceptRichText(false);
    this->ui.input->setFocus();

    this->pluginLoadedConn =
        getApp()->getPlugins()->onPluginLoaded.connect([this](Plugin *plug) {
            if (plug->id == this->id)
            {
                this->setPlugin(plug);
            }
        });

    this->updatePinned();
    this->themeChangedEvent();
    this->tryUpdate();
}

void PluginRepl::themeChangedEvent()
{
    this->updateFont();
    if (getTheme()->isLightTheme())
    {
        this->charFormats.self.setForeground(QColor(0x505050));
        this->charFormats.warning.setForeground(QColor(0x715100));
        this->blockFormats.warning.setBackground(QColor(0xfffbd6));
        this->charFormats.error.setForeground(QColor(0xa4000f));
        this->blockFormats.error.setBackground(QColor(0xfdf2f5));
    }
    else
    {
        this->charFormats.self.setForeground(QColor(0xa0a0a0));
        this->charFormats.warning.setForeground(QColor(0xfce2a1));
        this->blockFormats.warning.setBackground(QColor(0x42381f));
        this->charFormats.error.setForeground(QColor(0xffb3d2));
        this->blockFormats.error.setBackground(QColor(0x4b2f36));
    }

    auto pal = this->palette();
    pal.setColor(QPalette::Window,
                 getTheme()->tabs.selected.backgrounds.regular);
    pal.setColor(QPalette::Base, getTheme()->splits.background);
    pal.setColor(QPalette::Text, getTheme()->window.text);

    this->ui.output->setPalette(pal);

    pal.setColor(QPalette::Base, getTheme()->splits.input.background);
    this->ui.input->setPalette(pal);
}

void PluginRepl::tryRun(QString code)
{
    if (!this->plugin)
    {
        this->log(lua::api::LogLevel::Critical, "Plugin not loaded.");
        return;
    }

    this->log({}, u"> "_s + code);

    bool addedReturn = false;
    size_t maxItems = 10;

    if (code.startsWith('!'))
    {
        maxItems = std::numeric_limits<size_t>::max();
        auto v = QStringView(code).sliced(1).trimmed();
        auto nChars = v.constData() - code.constData();
        code.remove(0, nChars);
    }

    if (!code.startsWith(u"return "))
    {
        code.prepend(u"return ");
        addedReturn = true;
    }

    sol::protected_function_result res;
    try
    {
        std::string u8code = code.toStdString();
        std::string chunkName = "<inline code>";
        auto result =
            this->plugin->state().load(u8code, chunkName, sol::load_mode::text);

        if (!result.valid() && addedReturn)
        {
            u8code = u8code.substr(7);  // "return "
            result = this->plugin->state().load(u8code, chunkName,
                                                sol::load_mode::text);
        }

        if (!result.valid())
        {
            auto err = result.get<sol::error>();
            this->log(lua::api::LogLevel::Critical, err.what());
            return;
        }

        auto fn = result.get<sol::optional<sol::protected_function>>();
        if (!fn)
        {
            this->log(lua::api::LogLevel::Critical,
                      u"Code didn't result in a callable function."_s);
            return;
        }

        sol::protected_function_result res = (*fn)();
        this->logResult(res, {
                                 .maxItems = maxItems,
                             });
    }
    catch (const sol::error &err)
    {
        this->log(lua::api::LogLevel::Critical, err.what());
    }
}

void PluginRepl::logResult(const sol::protected_function_result &res,
                           const LogOptions &opts)
{
    if (!res.valid())
    {
        this->log(lua::api::LogLevel::Critical, lua::errorResultToString(res));
        return;
    }
    if (res.return_count() == 0)
    {
        return;
    }

    QString msg;
    for (auto it : res)
    {
        if (!msg.isEmpty())
        {
            msg.append(' ');
        }
        stringify(it, msg, opts.maxItems);
    }
    this->log(lua::api::LogLevel::Info, msg);
}

void PluginRepl::log(std::optional<lua::api::LogLevel> level,
                     const QString &text)
{
    auto [charFmt, blockFmt] = [&] {
        if (!level)
        {
            return std::tie(this->charFormats.self, this->blockFormats.self);
        }

        switch (*level)
        {
            case lua::api::LogLevel::Debug:
                return std::tie(this->charFormats.debug,
                                this->blockFormats.debug);
            case lua::api::LogLevel::Warning:
                return std::tie(this->charFormats.warning,
                                this->blockFormats.warning);
            case lua::api::LogLevel::Critical:
                return std::tie(this->charFormats.error,
                                this->blockFormats.error);
            case lua::api::LogLevel::Info:
            default:
                return std::tie(this->charFormats.info,
                                this->blockFormats.info);
        }
    }();

    auto *bar = this->ui.output->verticalScrollBar();
    bool atBottom = bar != nullptr && bar->value() == bar->maximum();
    auto cursor = this->ui.output->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (this->ui.output->document()->isEmpty())
    {
        cursor.setBlockFormat(blockFmt);
        cursor.setBlockCharFormat(charFmt);
    }
    else
    {
        cursor.insertBlock(blockFmt, charFmt);
    }
    cursor.insertText(text);

    if (atBottom)
    {
        bar->setValue(bar->maximum());
    }
}

void PluginRepl::tryUpdate()
{
    auto it = getApp()->getPlugins()->plugins().find(this->id);
    if (it == getApp()->getPlugins()->plugins().end())
    {
        return;
    }
    if (!PluginController::isPluginEnabled(this->id))
    {
        return;
    }
    this->setPlugin(it->second.get());
}

void PluginRepl::setPlugin(Plugin *plugin)
{
    this->plugin = plugin;

    if (!plugin)
    {
        this->pluginDestroyConn.release();
        this->pluginLogConn.release();
        return;
    }

    this->pluginDestroyConn = this->plugin->onUnloaded.connect([this] {
        this->setPlugin(nullptr);
        this->log({}, u"Unloaded."_s);
    });
    this->pluginLogConn =
        this->plugin->onLog.connect([this](auto level, const auto &text) {
            this->log(level, text);
        });

    this->log({}, u"Loaded."_s);
}

QFont PluginRepl::currentFont()
{
    auto family = getSettings()->pluginRepl.fontFamily.getValue();
    auto style = getSettings()->pluginRepl.fontStyle.getValue();
    auto fontSize = getSettings()->pluginRepl.fontSize.getValue();
    if (family.isEmpty())
    {
        auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setStyleName(style);
        font.setPointSize(fontSize);
        return font;
    }

    return QFontDatabase::font(family, style, fontSize);
}

void PluginRepl::updateFont()
{
    this->font = PluginRepl::currentFont();
    this->ui.input->setFont(this->font);
    this->ui.output->setFont(this->font);

    // Set the tab width to be at least 4 spaces.
    //
    // setTabStopDistance:
    // > Do not set a value less than the horizontalAdvance() of the
    // > QChar::VisualTabCharacter character, otherwise the tab-character will
    // > be drawn incompletely.
    QFontMetricsF metrics(this->font);
    auto tabCharWidth = metrics.horizontalAdvance(QChar::VisualTabCharacter);
    auto spaceWidth = metrics.horizontalAdvance(QChar::Space);
    auto tabDistance = std::max(tabCharWidth, spaceWidth * 4.F);

    this->ui.input->setTabStopDistance(tabDistance);
    this->ui.output->setTabStopDistance(tabDistance);
}

void PluginRepl::updatePinned()
{
    this->setTopMost(this->isPinned);
    if (this->isPinned)
    {
        this->ui.pin->setSource(this->ui.pinEnabledSource_);
    }
    else
    {
        this->ui.pin->setSource(this->ui.pinDisabledSource_);
    }
}

}  // namespace chatterino

#    include "PluginRepl.moc"

#endif
