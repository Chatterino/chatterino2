#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/notebook.hpp"
#include "widgets/titlebar.hpp"

//#ifdef USEWINSDK
//#include <platform/borderless/qwinwidget.h>
//#endif

#include <boost/property_tree/ptree.hpp>
#include <pajlada/settings/serialize.hpp>
#include <pajlada/settings/settingdata.hpp>

namespace chatterino {

class ChannelManager;
class ColorScheme;
class CompletionManager;

namespace widgets {

class Window : public BaseWidget
{
    Q_OBJECT

public:
    explicit Window(ChannelManager &_channelManager, ColorScheme &_colorScheme,
                    CompletionManager &_completionManager, bool isMainWindow);
    ~Window();

    void repaintVisibleChatWidgets(Channel *channel = nullptr);

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
    void loadDefaults();

    bool isLoaded() const;

    Notebook &getNotebook();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    float dpi;

    virtual void refreshTheme() override;

    ChannelManager &channelManager;
    ColorScheme &colorScheme;
    CompletionManager &completionManager;

    Notebook notebook;
    bool loaded = false;
    TitleBar titleBar;

    /*
    class QRectWrapper : public pajlada::Settings::ISettingData, public QRect
    {
    public:
        QRectWrapper()
            : QRect(-1, -1, -1, -1)
        {
        }

        pajlada::Signals::Signal<const QRectWrapper &> valueChanged;

        const QRectWrapper &getValueRef() const
        {
            return *this;
        }

        virtual rapidjson::Value marshalInto(rapidjson::Document &d) override
        {
            using namespace pajlada::Settings;

            rapidjson::Value obj(rapidjson::kObjectType);

            auto _x = serializeToJSON<int>::serialize(this->x(), d.GetAllocator());
            auto _y = serializeToJSON<int>::serialize(this->y(), d.GetAllocator());
            auto _width = serializeToJSON<int>::serialize(this->width(), d.GetAllocator());
            auto _height = serializeToJSON<int>::serialize(this->height(), d.GetAllocator());

            obj.AddMember("x", _x, d.GetAllocator());
            obj.AddMember("y", _y, d.GetAllocator());
            obj.AddMember("width", _width, d.GetAllocator());
            obj.AddMember("height", _height, d.GetAllocator());

            return obj;
        }

        virtual bool unmarshalFrom(rapidjson::Document &document) override
        {
            using namespace pajlada::Settings;

            auto vXp = this->getValueWithSuffix("/x", document);
            auto vYp = this->getValueWithSuffix("/y", document);
            auto vWidthp = this->getValueWithSuffix("/width", document);
            auto vHeightp = this->getValueWithSuffix("/height", document);
            if (vXp != nullptr) {
                this->setX(deserializeJSON<int>::deserialize(*vXp));
                this->filled = true;
            }
            if (vYp != nullptr) {
                this->setY(deserializeJSON<int>::deserialize(*vYp));
                this->filled = true;
            }
            if (vWidthp != nullptr) {
                this->setWidth(deserializeJSON<int>::deserialize(*vWidthp));
                this->filled = true;
            }
            if (vHeightp != nullptr) {
                this->setHeight(deserializeJSON<int>::deserialize(*vHeightp));
                this->filled = true;
            }

            return true;
        }

        virtual void registerDocument(rapidjson::Document &d) override
        {
            this->valueChanged.connect([this, &d](const auto &) {
                this->marshalInto(d);  //
            });
        }

        QRectWrapper &operator=(const QRect &rhs)
        {
            static_cast<QRect &>(*this) = rhs;

            return *this;
        }

        void setValue(const QRect &rhs)
        {
            static_cast<QRect &>(*this) = rhs;
        }
    };
    */

    // pajlada::Settings::Setting<QRectWrapper> windowGeometry;

    friend class Notebook;
};

}  // namespace widgets
}  // namespace chatterino
