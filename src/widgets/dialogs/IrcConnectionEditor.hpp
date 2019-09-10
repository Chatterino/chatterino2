#pragma once

#include <boost/optional.hpp>

#include "providers/irc/Irc2.hpp"
#include "widgets/BaseWindow.hpp"

namespace Ui {
class IrcConnectionEditor;
}

namespace chatterino {

struct IrcConnection_;

class IrcConnectionEditor : public QDialog
{
    Q_OBJECT

public:
    explicit IrcConnectionEditor(const IrcConnection_ &data, bool isAdd = false,
                                 QWidget *parent = nullptr);
    ~IrcConnectionEditor();

    IrcConnection_ data();

private:
    Ui::IrcConnectionEditor *ui_;
    IrcConnection_ data_;
};

}  // namespace chatterino
