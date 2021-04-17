#pragma once

#include "widgets/BaseWindow.hpp"

namespace chatterino {

class Split;

class FramelessEmbedWindow : public BaseWindow
{
public:
    FramelessEmbedWindow();

protected:
#ifdef USEWINSDK
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
    void showEvent(QShowEvent *event) override;
#endif

private:
    Split *split_{};
};

}  // namespace chatterino
