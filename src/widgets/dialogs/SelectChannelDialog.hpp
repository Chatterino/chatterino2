// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signal.hpp>
#include <QFocusEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>

#include <optional>

namespace chatterino::detail {

/// a radio button that checks itself when it receives focus
class AutoCheckedRadioButton : public QRadioButton
{
public:
    AutoCheckedRadioButton(const QString &label)
        : QRadioButton(label)
    {
    }

protected:
    void focusInEvent(QFocusEvent * /*event*/) override
    {
        this->setChecked(true);
    }
};

}  // namespace chatterino::detail

namespace chatterino {

class EditableModelView;
class IndirectChannel;
class MicroNotebook;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

namespace lua::api::channelproviders {
struct ArgumentSpec;
}  // namespace lua::api::channelproviders

class SelectChannelDialog final : public BaseWindow
{
public:
    SelectChannelDialog(QWidget *parent = nullptr);

    void setSelectedChannel(std::optional<IndirectChannel> channel_);
    IndirectChannel getSelectedChannel() const;
    bool hasSeletedChannel() const;

    pajlada::Signals::NoArgSignal closed;

protected:
    void closeEvent(QCloseEvent *event) override;
    void themeChangedEvent() override;
    void scaleChangedEvent(float newScale) override;

private:
    class EventFilter : public QObject
    {
    public:
        SelectChannelDialog *dialog;

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
    };

    struct {
        detail::AutoCheckedRadioButton *channel;
        QLabel *channelLabel;
        QLineEdit *channelName;

        detail::AutoCheckedRadioButton *whispers;
        QLabel *whispersLabel;

        detail::AutoCheckedRadioButton *mentions;
        QLabel *mentionsLabel;

        detail::AutoCheckedRadioButton *watching;
        QLabel *watchingLabel;

        detail::AutoCheckedRadioButton *live;
        QLabel *liveLabel;

        detail::AutoCheckedRadioButton *automod;
        QLabel *automodLabel;

        MicroNotebook *notebook;
        QWidget *twitchPage;
        QWidget *pluginPage;

        QComboBox *pluginProviderBox;
        QFormLayout *pluginProviderArgumentsLayout;
    } ui_{};

    EventFilter tabFilter_;

    ChannelPtr selectedChannel_;
    bool hasSelectedChannel_ = false;

    struct PluginControlFunctions {
        std::function<void(const QJsonObject &)> applyFromArguments;
        std::function<void(QJsonObject &)> applyToArguments;
    };
    std::vector<PluginControlFunctions> pluginControlCallbacks;

    void ok();
    friend class EventFilter;

    void addShortcuts() override;

    void refreshChannelProviderWidgets();
    QJsonObject extractChannelProviderArguments() const;
    void applyChannelProviderArguments(const QJsonObject &arguments);

    QWidget *createChannelProviderArgumentWidget(
        const lua::api::channelproviders::ArgumentSpec &arg);
};

}  // namespace chatterino
