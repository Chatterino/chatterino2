#include "AdvancedPage.hpp"

#include "Application.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

namespace chatterino {

AdvancedPage::AdvancedPage()
    : SettingsPage("Advanced", ":/settings/advanced.svg")
{
    LayoutCreator<AdvancedPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();

    {
        auto layout = tabs.appendTab(new QVBoxLayout, "Cache");
        auto folderLabel = layout.emplace<QLabel>();

        folderLabel->setTextFormat(Qt::RichText);
        folderLabel->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                             Qt::LinksAccessibleByKeyboard |
                                             Qt::LinksAccessibleByKeyboard);
        folderLabel->setOpenExternalLinks(true);

        getSettings()->cachePath.connect([folderLabel](const auto &,
                                                       auto) mutable {
            QString newPath = getPaths()->cacheDirectory();

            QString pathShortened = "Cache saved at <a href=\"file:///" +
                                    newPath +
                                    "\"><span style=\"color: white;\">" +
                                    shortenString(newPath, 50) + "</span></a>";

            folderLabel->setText(pathShortened);
            folderLabel->setToolTip(newPath);
        });

        layout->addStretch(1);

        auto selectDir = layout.emplace<QPushButton>("Set custom cache folder");

        QObject::connect(
            selectDir.getElement(), &QPushButton::clicked, this, [this] {
                auto dirName = QFileDialog::getExistingDirectory(this);

                getSettings()->cachePath = dirName;
            });

        auto resetDir =
            layout.emplace<QPushButton>("Reset custom cache folder");
        QObject::connect(resetDir.getElement(), &QPushButton::clicked, this,
                         []() mutable {
                             getSettings()->cachePath = "";  //
                         });
    }
        // Logs end


        // Timeoutbuttons
    {
        auto timeoutLayout = tabs.appendTab(new QVBoxLayout, "Timeouts");
        auto texts = timeoutLayout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto infoLabel = texts.emplace<QLabel>();
            infoLabel->setText(
                "Customize your timeout buttons in seconds (s), "
                "minutes (m), hours (h), days (d) or weeks (w).");

            infoLabel->setAlignment(Qt::AlignCenter);

            auto maxLabel = texts.emplace<QLabel>();
            maxLabel->setText("(maximum timeout duration = 2 w)");
            maxLabel->setAlignment(Qt::AlignCenter);
        }
        texts->setContentsMargins(0, 0, 0, 15);
        texts->setSizeConstraint(QLayout::SetMaximumSize);

        const auto valueChanged = [=] {
            const auto index = QObject::sender()->objectName().toInt();

            const auto line = this->durationInputs_[index];
            const auto duration = line->text().toInt();
            const auto unit = this->unitInputs_[index]->currentText();

            // safety mechanism for setting days and weeks
            if (unit == "d" && duration > 14)
            {
                line->setText("14");
                return;
            }
            else if (unit == "w" && duration > 2)
            {
                line->setText("2");
                return;
            }

            auto timeouts = getSettings()->timeoutButtons.getValue();
            timeouts[index] = TimeoutButton{ unit, duration };
            getSettings()->timeoutButtons.setValue(timeouts);
        };

        // build one line for each customizable button
        auto i = 0;
        for (const auto tButton : getSettings()->timeoutButtons.getValue())
        {
            const auto buttonNumber = QString::number(i);
            auto timeout = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();

            auto buttonLabel = timeout.emplace<QLabel>();
            buttonLabel->setText(QString("Button %1: ").arg(++i));

            auto *lineEditDurationInput = new QLineEdit();
            lineEditDurationInput->setObjectName(buttonNumber);
            lineEditDurationInput->setValidator(
                new QIntValidator(1, 99, this));
            lineEditDurationInput->setText(
                QString::number(tButton.second));
            lineEditDurationInput->setAlignment(Qt::AlignRight);
            lineEditDurationInput->setMaximumWidth(30);
            timeout.append(lineEditDurationInput);

            auto *timeoutDurationUnit = new QComboBox();
            timeoutDurationUnit->setObjectName(buttonNumber);
            timeoutDurationUnit->addItems({ "s", "m", "h", "d", "w" });
            timeoutDurationUnit->setCurrentText(tButton.first);
            timeout.append(timeoutDurationUnit);

            QObject::connect(lineEditDurationInput,
                             &QLineEdit::textChanged, this,
                             valueChanged);

            QObject::connect(timeoutDurationUnit,
                             &QComboBox::currentTextChanged, this,
                             valueChanged);

            timeout->addStretch();

            this->durationInputs_.push_back(lineEditDurationInput);
            this->unitInputs_.push_back(timeoutDurationUnit);

            timeout->setContentsMargins(40, 0, 0, 0);
            timeout->setSizeConstraint(QLayout::SetMaximumSize);
        }
        timeoutLayout->addStretch();
    }
    // Timeoutbuttons end
}

}  // namespace chatterino
