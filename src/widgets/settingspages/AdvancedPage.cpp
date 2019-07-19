#include "AdvancedPage.hpp"

#include "Application.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
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

        // Logs end

        // Timeoutbuttons
        QStringList units = QStringList();
        units.append("s");
        units.append("m");
        units.append("h");
        units.append("d");
        units.append("w");

        QStringList initDurationsPerUnit;
        initDurationsPerUnit = QStringList();
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit1));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit2));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit3));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit4));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit5));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit6));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit7));
        initDurationsPerUnit.append(
            QString(getSettings()->timeoutDurationPerUnit8));

        QStringList::iterator itDurationPerUnit;
        itDurationPerUnit = initDurationsPerUnit.begin();

        QStringList initUnits;
        initUnits = QStringList();
        initUnits.append(QString(getSettings()->timeoutDurationUnit1));
        initUnits.append(QString(getSettings()->timeoutDurationUnit2));
        initUnits.append(QString(getSettings()->timeoutDurationUnit3));
        initUnits.append(QString(getSettings()->timeoutDurationUnit4));
        initUnits.append(QString(getSettings()->timeoutDurationUnit5));
        initUnits.append(QString(getSettings()->timeoutDurationUnit6));
        initUnits.append(QString(getSettings()->timeoutDurationUnit7));
        initUnits.append(QString(getSettings()->timeoutDurationUnit8));

        QStringList::iterator itUnit;
        itUnit = initUnits.begin();

        for (int i = 0; i < 8; i++)
        {
            durationInputs.append(new QLineEdit());
            unitInputs.append(new QComboBox());
        }
        itDurationInput = durationInputs.begin();
        itUnitInput = unitInputs.begin();

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

        // build one line for each customizable button
        for (int i = 0; i < 8; i++)
        {
            auto timeout = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
            {
                auto buttonLabel = timeout.emplace<QLabel>();
                buttonLabel->setText("Button " + QString::number(i + 1) + ": ");

                QLineEdit *lineEditDurationInput = *itDurationInput;
                lineEditDurationInput->setObjectName(QString::number(i));
                lineEditDurationInput->setValidator(
                    new QIntValidator(1, 99, this));
                lineEditDurationInput->setText(*itDurationPerUnit);
                lineEditDurationInput->setAlignment(Qt::AlignRight);
                lineEditDurationInput->setMaximumWidth(30);
                timeout.append(lineEditDurationInput);

                QComboBox *timeoutDurationUnit = *itUnitInput;
                timeoutDurationUnit->setObjectName(QString::number(i));
                timeoutDurationUnit->addItems(units);
                timeoutDurationUnit->setCurrentText(*itUnit);
                timeout.append(timeoutDurationUnit);

                QObject::connect(lineEditDurationInput, &QLineEdit::textChanged,
                                 this, &AdvancedPage::timeoutDurationChanged);

                QObject::connect(timeoutDurationUnit,
                                 &QComboBox::currentTextChanged, this,
                                 &AdvancedPage::timeoutUnitChanged);

                auto secondsLabel = timeout.emplace<QLabel>();
                timeout->addStretch();
            }
            timeout->setContentsMargins(40, 0, 0, 0);
            timeout->setSizeConstraint(QLayout::SetMaximumSize);

            ++itDurationPerUnit;
            ++itUnit;
            ++itDurationInput;
            ++itUnitInput;
        }
        timeoutLayout->addStretch();
    }
    // Timeoutbuttons end
}

void AdvancedPage::timeoutDurationChanged(const QString &newDuration)
{
    QObject *sender = QObject::sender();
    int index = sender->objectName().toInt();

    itDurationInput = durationInputs.begin() + index;
    QLineEdit *durationPerUnit = *itDurationInput;

    itUnitInput = unitInputs.begin() + index;
    QComboBox *cbUnit = *itUnitInput;
    QString unit = cbUnit->currentText();

    int valueInUnit = newDuration.toInt();
    int valueInSec;

    if (unit == "s")
    {
        valueInSec = valueInUnit;
    }
    else if (unit == "m")
    {
        valueInSec = valueInUnit * 60;
    }
    else if (unit == "h")
    {
        valueInSec = valueInUnit * 60 * 60;
    }
    else if (unit == "d")
    {
        if (valueInUnit > 14)
        {
            durationPerUnit->setText("14");
            return;
        }
        valueInSec = valueInUnit * 24 * 60 * 60;
    }
    else if (unit == "w")
    {
        if (valueInUnit > 2)
        {
            durationPerUnit->setText("2");
            return;
        }
        valueInSec = valueInUnit * 7 * 24 * 60 * 60;
    }

    switch (index)
    {
        case 0:
            getSettings()->timeoutDurationPerUnit1 = newDuration;
            getSettings()->timeoutDurationInSec1 = valueInSec;
            break;
        case 1:
            getSettings()->timeoutDurationPerUnit2 = newDuration;
            getSettings()->timeoutDurationInSec2 = valueInSec;
            break;
        case 2:
            getSettings()->timeoutDurationPerUnit3 = newDuration;
            getSettings()->timeoutDurationInSec3 = valueInSec;
            break;
        case 3:
            getSettings()->timeoutDurationPerUnit4 = newDuration;
            getSettings()->timeoutDurationInSec4 = valueInSec;
            break;
        case 4:
            getSettings()->timeoutDurationPerUnit5 = newDuration;
            getSettings()->timeoutDurationInSec5 = valueInSec;
            break;
        case 5:
            getSettings()->timeoutDurationPerUnit6 = newDuration;
            getSettings()->timeoutDurationInSec6 = valueInSec;
            break;
        case 6:
            getSettings()->timeoutDurationPerUnit7 = newDuration;
            getSettings()->timeoutDurationInSec7 = valueInSec;
            break;
        case 7:
            getSettings()->timeoutDurationPerUnit8 = newDuration;
            getSettings()->timeoutDurationInSec8 = valueInSec;
            break;
    }
}

void AdvancedPage::timeoutUnitChanged(const QString &newUnit)
{
    QObject *sender = QObject::sender();
    int index = sender->objectName().toInt();

    switch (index)
    {
        case 0:
            getSettings()->timeoutDurationUnit1 = newUnit;
            break;
        case 1:
            getSettings()->timeoutDurationUnit2 = newUnit;
            break;
        case 2:
            getSettings()->timeoutDurationUnit3 = newUnit;
            break;
        case 3:
            getSettings()->timeoutDurationUnit4 = newUnit;
            break;
        case 4:
            getSettings()->timeoutDurationUnit5 = newUnit;
            break;
        case 5:
            getSettings()->timeoutDurationUnit6 = newUnit;
            break;
        case 6:
            getSettings()->timeoutDurationUnit7 = newUnit;
            break;
        case 7:
            getSettings()->timeoutDurationUnit8 = newUnit;
            break;
    }

    itDurationInput = durationInputs.begin() + index;
    QLineEdit *durationPerUnit = *itDurationInput;

    AdvancedPage::timeoutDurationChanged(durationPerUnit->text());
}

}  // namespace chatterino
