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

#    if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;
#    else
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
#    endif

    void showEvent(QShowEvent *event) override;
#endif

private:
    Split *split_{};
};

}  // namespace chatterino
