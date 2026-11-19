#include "widgets/splits/Split.hpp"

#include "common/WindowDescriptors.hpp"

#include <chatterino-embed/Split.hpp>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace chatterino::embed {

Split::Split(QWidget *parent)
    : QWidget(parent)
    , split_(new chatterino::Split(this))
{
    std::ignore = this->split_->channelChanged.connect([this] {
        this->channelChanged();
    });
    std::ignore = this->split_->actionRequested.connect(
        [this](chatterino::Split::Action action) {
            using Action = chatterino::Split::Action;
            switch (action)
            {
                case Action::Delete:
                    this->closeRequested();
                    break;

                case Action::RefreshTab:
                case Action::ResetMouseStatus:
                case Action::AppendNewSplit:
                case Action::SelectSplitLeft:
                case Action::SelectSplitRight:
                case Action::SelectSplitAbove:
                case Action::SelectSplitBelow:
                    break;  // unhandled
            }
        });
    this->setContentsMargins({});
}

void Split::deserializeData(QByteArrayView data)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const auto obj = QJsonValue::fromJson(data).toObject();
#else
    const auto obj = QJsonDocument::fromJson(data.toByteArray()).object();
#endif

    auto descriptor = SplitDescriptor::loadFromJSON(obj);
    this->split_->setChannel(descriptor.decodeChannel());
    this->split_->setModerationMode(descriptor.moderationMode_);
    this->split_->setFilters(descriptor.filters_);
    this->split_->setCheckSpellingOverride(descriptor.spellCheckOverride);
}

QByteArray Split::serializeData()
{
    return QJsonDocument(this->split_->buildDescriptor().toJson())
        .toJson(QJsonDocument::Compact);
}

QString Split::channelName() const
{
    return this->split_->getChannel()->getDisplayName();
}

void Split::resizeEvent(QResizeEvent * /* event */)
{
    this->split_->setGeometry(this->rect());
}

}  // namespace chatterino::embed
