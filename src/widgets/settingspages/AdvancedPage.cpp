#include "AdvancedPage.hpp"

#include "Application.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "debug/Log.hpp"
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

        std::list<int> initDurationsInSec;
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec1));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec2));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec3));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec4));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec5));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec6));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec7));
        initDurationsInSec.push_back(int(getSettings()->timeoutDurationInSec8));

        std::list<int>::iterator itDurationInSec;
        itDurationInSec = initDurationsInSec.begin();

        for (int i = 0; i < 8; i++)
        {
            durationInputs.append(new QLineEdit());
            unitInputs.append(new QComboBox());
            durationsCalculated.append(new QSpinBox());
        }
        itDurationInput = durationInputs.begin();
        itUnitInput = unitInputs.begin();
        itDurationsCalculated = durationsCalculated.begin();

        auto timeoutLayout = tabs.appendTab(new QVBoxLayout, "Timeouts");
        auto infoLabel = timeoutLayout.emplace<QLabel>();
        infoLabel->setText("Customize your timeout buttons in seconds (s), "
                           "minutes (m), hours (h), days (d) or weeks (w).");
        auto maxLabel = timeoutLayout.emplace<QLabel>();
        maxLabel->setText("Maximum timeout duration is 2 weeks = 14 days = 336 "
                          "hours = 20,160 minutes =  1,209,600 seconds.");
        auto allowedLabel = timeoutLayout.emplace<QLabel>();
        allowedLabel->setText(
            "For simplicity only values from 1 to 99 are allowed here. Using "
            "the units accordingly is highly recommended.");
        auto commandLabel = timeoutLayout.emplace<QLabel>();
        commandLabel->setText(
            "For abitrary timeout durations use the /timeout command.");

        // build one line for each customizable button
        for (int i = 0; i < 8; i++)
        {
            auto timeout = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
            {
                QLineEdit *lineEditDurationInput = *itDurationInput;
                lineEditDurationInput->setObjectName(QString::number(i));
                lineEditDurationInput->setValidator(
                    new QIntValidator(1, 99, this));
                lineEditDurationInput->setText(*itDurationPerUnit);
                lineEditDurationInput->setAlignment(Qt::AlignRight);
                timeout.append(lineEditDurationInput);

                QComboBox *timeoutDurationUnit = *itUnitInput;
                timeoutDurationUnit->setObjectName(QString::number(i));
                timeoutDurationUnit->addItems(units);
                timeoutDurationUnit->setCurrentText(*itUnit);
                timeout.append(timeoutDurationUnit);

                QSpinBox *spinBoxDurationCalculated = *itDurationsCalculated;
                spinBoxDurationCalculated->setObjectName(QString::number(i));
                spinBoxDurationCalculated->setRange(1, 1209600);
                spinBoxDurationCalculated->setValue(*itDurationInSec);
                spinBoxDurationCalculated->setReadOnly(true);
                spinBoxDurationCalculated->setAlignment(Qt::AlignRight);
                timeout.append(spinBoxDurationCalculated);

                QObject::connect(lineEditDurationInput, &QLineEdit::textChanged,
                                 this, &AdvancedPage::timeoutDurationChanged);

                QObject::connect(timeoutDurationUnit,
                                 &QComboBox::currentTextChanged, this,
                                 &AdvancedPage::timeoutUnitChanged);

                QObject::connect(spinBoxDurationCalculated,
                                 qOverload<int>(&QSpinBox::valueChanged), this,
                                 &AdvancedPage::calculatedDurationChanged);

                auto secondsLabel = timeout.emplace<QLabel>();
                secondsLabel->setText("seconds");
            }

            ++itDurationPerUnit;
            ++itUnit;
            ++itDurationInSec;
            ++itDurationInput;
            ++itUnitInput;
            ++itDurationsCalculated;
        }
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

    itDurationsCalculated = durationsCalculated.begin() + index;
    QSpinBox *sbDurationInSec = *itDurationsCalculated;

    int valueInUnit = newDuration.toInt();
    int valueInSec;
    log(newDuration);
    log(unit);
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

    sbDurationInSec->setValue(valueInSec);

    switch (index)
    {
        case 0:
            getSettings()->timeoutDurationPerUnit1 = newDuration;
            break;
        case 1:
            getSettings()->timeoutDurationPerUnit2 = newDuration;
            break;
        case 2:
            getSettings()->timeoutDurationPerUnit3 = newDuration;
            break;
        case 3:
            getSettings()->timeoutDurationPerUnit4 = newDuration;
            break;
        case 4:
            getSettings()->timeoutDurationPerUnit5 = newDuration;
            break;
        case 5:
            getSettings()->timeoutDurationPerUnit6 = newDuration;
            break;
        case 6:
            getSettings()->timeoutDurationPerUnit7 = newDuration;
            break;
        case 7:
            getSettings()->timeoutDurationPerUnit8 = newDuration;
            break;
    }
}
void AdvancedPage::calculatedDurationChanged(const int &newDurationInSec)
{
    QObject *sender = QObject::sender();
    int index = sender->objectName().toInt();

    switch (index)
    {
        case 0:
            getSettings()->timeoutDurationInSec1 = newDurationInSec;
            break;
        case 1:
            getSettings()->timeoutDurationInSec2 = newDurationInSec;
            break;
        case 2:
            getSettings()->timeoutDurationInSec3 = newDurationInSec;
            break;
        case 3:
            getSettings()->timeoutDurationInSec4 = newDurationInSec;
            break;
        case 4:
            getSettings()->timeoutDurationInSec5 = newDurationInSec;
            break;
        case 5:
            getSettings()->timeoutDurationInSec6 = newDurationInSec;
            break;
        case 6:
            getSettings()->timeoutDurationInSec7 = newDurationInSec;
            break;
        case 7:
            getSettings()->timeoutDurationInSec8 = newDurationInSec;
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
