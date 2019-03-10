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
        const QJsonObject& obj, Application& app, FlexItem* parent)
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

            parent->addChild(item);
        }
        else if (type == "horizontal" || type == "vertical")
        {
            auto vertical = type == "vertical";
            auto direction = vertical ? Direction::Below : Direction::Right;

            item = std::make_shared<FlexItem>(
                vertical ? FlexItem::Column : FlexItem::Row);

            for (QJsonValue _val : obj.value("items").toArray())
            {
                auto _obj = _val.toObject();

#if 0
                auto _type = _obj.value("type");
                if (_type == "split")
                {
                    auto* split = new Split(this);
                    split->setChannel(WindowManager::decodeChannel(
                        _obj.value("data").toObject()));

                    Node* _node = new Node();
                    _node->parent_ = node;
                    _node->split_ = split;
                    _node->type_ = Node::_Split;

                    _node->flexH_ = _obj.value("flexh").toDouble(1.0);
                    _node->flexV_ = _obj.value("flexv").toDouble(1.0);
                    node->children_.emplace_back(_node);

                    this->addSplit(split);
                }
                else
                {
                    Node* _node = new Node();
                    _node->parent_ = node;
                    node->children_.emplace_back(_node);
                    this->decodeNodeRecusively(_obj, _node);
                }
#endif

                parent->addChild(item);
            }

#if 0
            for (int i = 0; i < 2; i++)
            {
                if (node->getChildren().size() < 2)
                {
                    auto* split = new Split(this);
                    split->setChannel(WindowManager::decodeChannel(
                        obj.value("data").toObject()));

                    this->insertSplit(split, direction, node);
                }
            }
#endif
        }

        item->flexH = obj.value("flexh").toDouble(1.0);
        item->flexV = obj.value("flexv").toDouble(1.0);

        return item;
    }

    void SplitContainer::deserialize(const QJsonObject& obj, Application& app)
    {
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
