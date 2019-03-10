#include "SplitContainer.hpp"

#include <Application.hpp>
#include <Provider.hpp>

#include "ui/FlexLayout.Private.hpp"
#include "ui/FlexLayout.hpp"
#include "ui/Split.hpp"

namespace chatterino::ui
{
    SplitContainer::SplitContainer(Application& app)
        : app_(app)
    {
        this->flex_ = new FlexLayout();
        this->setLayout(this->flex_);
    }

    inline std::shared_ptr<FlexItem> deserializeRec(
        const QJsonObject& obj, Application& app, FlexItem& parent)
    {
        auto item = std::shared_ptr<FlexItem>();

        auto type = obj.value("type").toString();

        if (type == "split")
        {
            auto split = new Split(app);
            item = std::make_shared<FlexItem>(split);

            auto provName = obj.value("provider").toString("twitch");
            if (auto provider = app.provider(provName))
                split->setRoom(provider->addRoom(obj.value("data").toObject()));
            else
                assert(false);

            parent.addChild(item);
        }
        else if (type == "horizontal" || type == "vertical")
        {
            auto vertical = type == "vertical";

            item = std::make_shared<FlexItem>(
                vertical ? FlexItem::Column : FlexItem::Row);

            for (QJsonValue _val : obj.value("items").toArray())
            {
                auto _obj = _val.toObject();

                deserializeRec(_obj, app, *item);
            }
            parent.addChild(item);
        }

        item->flex = obj.value("flex").toDouble(1.0);

        return item;
    }

    void SplitContainer::deserialize(const QJsonObject& obj, Application& app)
    {
        auto root = std::make_shared<FlexItem>(FlexItem::Row);
        deserializeRec(obj, app, *root);
        this->flex_->setRoot(root);

#if 0
            // fallback load splits (old)
            int colNr = 0;
            for (QJsonValue column_val : tab_obj.value("splits").toArray())
            {
                for (QJsonValue split_val : column_val.toArray())
                {
                    Split* split = new Split(page);

                    QJsonObject split_obj = split_val.toObject();
                    split->setChannel(decodeChannel(split_obj));

                    page->appendSplit(split);
                }
                colNr++;
            }
#endif
    }
}  // namespace chatterino::ui
