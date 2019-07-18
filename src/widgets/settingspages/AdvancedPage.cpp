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
        QStringList units = QStringList();
        units.append("s");
        units.append("m");
        units.append("h");
        units.append("d");
        units.append("w");

        auto timeoutLayout = tabs.appendTab(new QVBoxLayout, "Timeouts");
        auto infoLabel = timeoutLayout.emplace<QLabel>();
        infoLabel->setText("Customize your timeout buttons in seconds (s), "
                           "minutes (m), hours (h), days (d) or weeks (w).");
        auto maxLabel = timeoutLayout.emplace<QLabel>();
        maxLabel->setText("Maximum timeout duration is 2 weeks = 14 days = 336 "
                          "hours = 20,160 minutes =  1,209,600 seconds.");

        auto timeout1 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput1 = new QLineEdit;
            lineEditDurationInput1->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput1->setText(
                getSettings()->timeoutDurationPerUnit1);
            lineEditDurationInput1->setAlignment(Qt::AlignRight);
            timeout1.append(lineEditDurationInput1);

            QComboBox *timeoutDurationUnit1 = new QComboBox();
            timeoutDurationUnit1->addItems(units);
            timeoutDurationUnit1->setCurrentText(
                getSettings()->timeoutDurationUnit1);
            timeout1.append(timeoutDurationUnit1);

            QSpinBox *spinBoxDurationCalculated1 = new QSpinBox;
            spinBoxDurationCalculated1->setRange(1, 1209600);
            spinBoxDurationCalculated1->setValue(
                getSettings()->timeoutDurationInSec1);
            spinBoxDurationCalculated1->setReadOnly(true);
            spinBoxDurationCalculated1->setAlignment(Qt::AlignRight);
            timeout1.append(spinBoxDurationCalculated1);

            QObject::connect(lineEditDurationInput1, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput1,
                                     getSettings()->timeoutDurationUnit1,
                                     *spinBoxDurationCalculated1,
                                     getSettings()->timeoutDurationPerUnit1);
                             });

            QObject::connect(spinBoxDurationCalculated1,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated1->value(),
                                     getSettings()->timeoutDurationInSec1);
                             });

            QObject::connect(timeoutDurationUnit1,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit1->currentText(),
                                     getSettings()->timeoutDurationUnit1);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput1,
                                     getSettings()->timeoutDurationUnit1,
                                     *spinBoxDurationCalculated1,
                                     getSettings()->timeoutDurationPerUnit1);
                             });

            auto secondsLabel = timeout1.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }

        auto timeout2 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput2 = new QLineEdit;
            lineEditDurationInput2->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput2->setText(
                getSettings()->timeoutDurationPerUnit2);
            lineEditDurationInput2->setAlignment(Qt::AlignRight);
            timeout2.append(lineEditDurationInput2);

            QComboBox *timeoutDurationUnit2 = new QComboBox();
            timeoutDurationUnit2->addItems(units);
            timeoutDurationUnit2->setCurrentText(
                getSettings()->timeoutDurationUnit2);
            timeout2.append(timeoutDurationUnit2);

            QSpinBox *spinBoxDurationCalculated2 = new QSpinBox;
            spinBoxDurationCalculated2->setRange(1, 1209600);
            spinBoxDurationCalculated2->setValue(
                getSettings()->timeoutDurationInSec2);
            spinBoxDurationCalculated2->setReadOnly(true);
            spinBoxDurationCalculated2->setAlignment(Qt::AlignRight);
            timeout2.append(spinBoxDurationCalculated2);

            QObject::connect(lineEditDurationInput2, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput2,
                                     getSettings()->timeoutDurationUnit2,
                                     *spinBoxDurationCalculated2,
                                     getSettings()->timeoutDurationPerUnit2);
                             });

            QObject::connect(spinBoxDurationCalculated2,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated2->value(),
                                     getSettings()->timeoutDurationInSec2);
                             });

            QObject::connect(timeoutDurationUnit2,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit2->currentText(),
                                     getSettings()->timeoutDurationUnit2);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput2,
                                     getSettings()->timeoutDurationUnit2,
                                     *spinBoxDurationCalculated2,
                                     getSettings()->timeoutDurationPerUnit2);
                             });

            auto secondsLabel = timeout2.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }

        auto timeout3 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput3 = new QLineEdit;
            lineEditDurationInput3->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput3->setText(
                getSettings()->timeoutDurationPerUnit3);
            lineEditDurationInput3->setAlignment(Qt::AlignRight);
            timeout3.append(lineEditDurationInput3);

            QComboBox *timeoutDurationUnit3 = new QComboBox();
            timeoutDurationUnit3->addItems(units);
            timeoutDurationUnit3->setCurrentText(
                getSettings()->timeoutDurationUnit3);
            timeout3.append(timeoutDurationUnit3);

            QSpinBox *spinBoxDurationCalculated3 = new QSpinBox;
            spinBoxDurationCalculated3->setRange(1, 1209600);
            spinBoxDurationCalculated3->setValue(
                getSettings()->timeoutDurationInSec3);
            spinBoxDurationCalculated3->setReadOnly(true);
            spinBoxDurationCalculated3->setAlignment(Qt::AlignRight);
            timeout3.append(spinBoxDurationCalculated3);

            QObject::connect(lineEditDurationInput3, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput3,
                                     getSettings()->timeoutDurationUnit3,
                                     *spinBoxDurationCalculated3,
                                     getSettings()->timeoutDurationPerUnit3);
                             });

            QObject::connect(spinBoxDurationCalculated3,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated3->value(),
                                     getSettings()->timeoutDurationInSec3);
                             });

            QObject::connect(timeoutDurationUnit3,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit3->currentText(),
                                     getSettings()->timeoutDurationUnit3);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput3,
                                     getSettings()->timeoutDurationUnit3,
                                     *spinBoxDurationCalculated3,
                                     getSettings()->timeoutDurationPerUnit3);
                             });

            auto secondsLabel = timeout3.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }
        auto timeout4 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput4 = new QLineEdit;
            lineEditDurationInput4->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput4->setText(
                getSettings()->timeoutDurationPerUnit4);
            lineEditDurationInput4->setAlignment(Qt::AlignRight);
            timeout4.append(lineEditDurationInput4);

            QComboBox *timeoutDurationUnit4 = new QComboBox();
            timeoutDurationUnit4->addItems(units);
            timeoutDurationUnit4->setCurrentText(
                getSettings()->timeoutDurationUnit4);
            timeout4.append(timeoutDurationUnit4);

            QSpinBox *spinBoxDurationCalculated4 = new QSpinBox;
            spinBoxDurationCalculated4->setRange(1, 1209600);
            spinBoxDurationCalculated4->setValue(
                getSettings()->timeoutDurationInSec4);
            spinBoxDurationCalculated4->setReadOnly(true);
            spinBoxDurationCalculated4->setAlignment(Qt::AlignRight);
            timeout4.append(spinBoxDurationCalculated4);

            QObject::connect(lineEditDurationInput4, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput4,
                                     getSettings()->timeoutDurationUnit4,
                                     *spinBoxDurationCalculated4,
                                     getSettings()->timeoutDurationPerUnit4);
                             });

            QObject::connect(spinBoxDurationCalculated4,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated4->value(),
                                     getSettings()->timeoutDurationInSec4);
                             });

            QObject::connect(timeoutDurationUnit4,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit4->currentText(),
                                     getSettings()->timeoutDurationUnit4);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput4,
                                     getSettings()->timeoutDurationUnit4,
                                     *spinBoxDurationCalculated4,
                                     getSettings()->timeoutDurationPerUnit4);
                             });

            auto secondsLabel = timeout4.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }
        auto timeout5 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput5 = new QLineEdit;
            lineEditDurationInput5->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput5->setText(
                getSettings()->timeoutDurationPerUnit5);
            lineEditDurationInput5->setAlignment(Qt::AlignRight);
            timeout5.append(lineEditDurationInput5);

            QComboBox *timeoutDurationUnit5 = new QComboBox();
            timeoutDurationUnit5->addItems(units);
            timeoutDurationUnit5->setCurrentText(
                getSettings()->timeoutDurationUnit5);
            timeout5.append(timeoutDurationUnit5);

            QSpinBox *spinBoxDurationCalculated5 = new QSpinBox;
            spinBoxDurationCalculated5->setRange(1, 1209600);
            spinBoxDurationCalculated5->setValue(
                getSettings()->timeoutDurationInSec5);
            spinBoxDurationCalculated5->setReadOnly(true);
            spinBoxDurationCalculated5->setAlignment(Qt::AlignRight);
            timeout5.append(spinBoxDurationCalculated5);

            QObject::connect(lineEditDurationInput5, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput5,
                                     getSettings()->timeoutDurationUnit5,
                                     *spinBoxDurationCalculated5,
                                     getSettings()->timeoutDurationPerUnit5);
                             });

            QObject::connect(spinBoxDurationCalculated5,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated5->value(),
                                     getSettings()->timeoutDurationInSec5);
                             });

            QObject::connect(timeoutDurationUnit5,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit5->currentText(),
                                     getSettings()->timeoutDurationUnit5);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput5,
                                     getSettings()->timeoutDurationUnit5,
                                     *spinBoxDurationCalculated5,
                                     getSettings()->timeoutDurationPerUnit5);
                             });

            auto secondsLabel = timeout5.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }
        auto timeout6 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput6 = new QLineEdit;
            lineEditDurationInput6->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput6->setText(
                getSettings()->timeoutDurationPerUnit6);
            lineEditDurationInput6->setAlignment(Qt::AlignRight);
            timeout6.append(lineEditDurationInput6);

            QComboBox *timeoutDurationUnit6 = new QComboBox();
            timeoutDurationUnit6->addItems(units);
            timeoutDurationUnit6->setCurrentText(
                getSettings()->timeoutDurationUnit6);
            timeout6.append(timeoutDurationUnit6);

            QSpinBox *spinBoxDurationCalculated6 = new QSpinBox;
            spinBoxDurationCalculated6->setRange(1, 1209600);
            spinBoxDurationCalculated6->setValue(
                getSettings()->timeoutDurationInSec6);
            spinBoxDurationCalculated6->setReadOnly(true);
            spinBoxDurationCalculated6->setAlignment(Qt::AlignRight);
            timeout6.append(spinBoxDurationCalculated6);

            QObject::connect(lineEditDurationInput6, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput6,
                                     getSettings()->timeoutDurationUnit6,
                                     *spinBoxDurationCalculated6,
                                     getSettings()->timeoutDurationPerUnit6);
                             });

            QObject::connect(spinBoxDurationCalculated6,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated6->value(),
                                     getSettings()->timeoutDurationInSec6);
                             });

            QObject::connect(timeoutDurationUnit6,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit6->currentText(),
                                     getSettings()->timeoutDurationUnit6);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput6,
                                     getSettings()->timeoutDurationUnit6,
                                     *spinBoxDurationCalculated6,
                                     getSettings()->timeoutDurationPerUnit6);
                             });

            auto secondsLabel = timeout6.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }

        auto timeout7 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput7 = new QLineEdit;
            lineEditDurationInput7->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput7->setText(
                getSettings()->timeoutDurationPerUnit7);
            lineEditDurationInput7->setAlignment(Qt::AlignRight);
            timeout7.append(lineEditDurationInput7);

            QComboBox *timeoutDurationUnit7 = new QComboBox();
            timeoutDurationUnit7->addItems(units);
            timeoutDurationUnit7->setCurrentText(
                getSettings()->timeoutDurationUnit7);
            timeout7.append(timeoutDurationUnit7);

            QSpinBox *spinBoxDurationCalculated7 = new QSpinBox;
            spinBoxDurationCalculated7->setRange(1, 1209600);
            spinBoxDurationCalculated7->setValue(
                getSettings()->timeoutDurationInSec7);
            spinBoxDurationCalculated7->setReadOnly(true);
            spinBoxDurationCalculated7->setAlignment(Qt::AlignRight);
            timeout7.append(spinBoxDurationCalculated7);

            QObject::connect(lineEditDurationInput7, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput7,
                                     getSettings()->timeoutDurationUnit7,
                                     *spinBoxDurationCalculated7,
                                     getSettings()->timeoutDurationPerUnit7);
                             });

            QObject::connect(spinBoxDurationCalculated7,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated7->value(),
                                     getSettings()->timeoutDurationInSec7);
                             });

            QObject::connect(timeoutDurationUnit7,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit7->currentText(),
                                     getSettings()->timeoutDurationUnit7);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput7,
                                     getSettings()->timeoutDurationUnit7,
                                     *spinBoxDurationCalculated7,
                                     getSettings()->timeoutDurationPerUnit7);
                             });

            auto secondsLabel = timeout7.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }
        auto timeout8 = timeoutLayout.emplace<QHBoxLayout>().withoutMargin();
        {
            QLineEdit *lineEditDurationInput8 = new QLineEdit;
            lineEditDurationInput8->setValidator(
                new QIntValidator(1, 1209600, this));
            lineEditDurationInput8->setText(
                getSettings()->timeoutDurationPerUnit8);
            lineEditDurationInput8->setAlignment(Qt::AlignRight);
            timeout8.append(lineEditDurationInput8);

            QComboBox *timeoutDurationUnit8 = new QComboBox();
            timeoutDurationUnit8->addItems(units);
            timeoutDurationUnit8->setCurrentText(
                getSettings()->timeoutDurationUnit8);
            timeout8.append(timeoutDurationUnit8);

            QSpinBox *spinBoxDurationCalculated8 = new QSpinBox;
            spinBoxDurationCalculated8->setRange(1, 1209600);
            spinBoxDurationCalculated8->setValue(
                getSettings()->timeoutDurationInSec8);
            spinBoxDurationCalculated8->setReadOnly(true);
            spinBoxDurationCalculated8->setAlignment(Qt::AlignRight);
            timeout8.append(spinBoxDurationCalculated8);

            QObject::connect(lineEditDurationInput8, &QLineEdit::textChanged,
                             [=] {
                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput8,
                                     getSettings()->timeoutDurationUnit8,
                                     *spinBoxDurationCalculated8,
                                     getSettings()->timeoutDurationPerUnit8);
                             });

            QObject::connect(spinBoxDurationCalculated8,
                             qOverload<int>(&QSpinBox::valueChanged), [=] {
                                 AdvancedPage::calculatedDurationChanged(
                                     spinBoxDurationCalculated8->value(),
                                     getSettings()->timeoutDurationInSec8);
                             });

            QObject::connect(timeoutDurationUnit8,
                             &QComboBox::currentTextChanged, [=] {
                                 AdvancedPage::timeoutUnitChanged(
                                     timeoutDurationUnit8->currentText(),
                                     getSettings()->timeoutDurationUnit8);

                                 AdvancedPage::timeoutDurationChanged(
                                     *lineEditDurationInput8,
                                     getSettings()->timeoutDurationUnit8,
                                     *spinBoxDurationCalculated8,
                                     getSettings()->timeoutDurationPerUnit8);
                             });

            auto secondsLabel = timeout8.emplace<QLabel>();
            secondsLabel->setText("seconds");
        }
    }
}

void AdvancedPage::timeoutDurationChanged(QLineEdit &durationPerUnit,
                                          const QString &unit,
                                          QSpinBox &durationInSec,
                                          QStringSetting settingDurationPerUnit)
{
    int valueInUnit = durationPerUnit.text().toInt();
    int valueInSec;
    if (unit == "s")
    {
        valueInSec = valueInUnit;
    }
    else if (unit == "m")
    {
        if (valueInUnit > 20160)
        {
            durationPerUnit.setText("20160");
            return;
        }
        valueInSec = valueInUnit * 60;
    }
    else if (unit == "h")
    {
        if (valueInUnit > 336)
        {
            durationPerUnit.setText("336");
            return;
        }
        valueInSec = valueInUnit * 60 * 60;
    }
    else if (unit == "d")
    {
        if (valueInUnit > 14)
        {
            durationPerUnit.setText("14");
            return;
        }
        valueInSec = valueInUnit * 24 * 60 * 60;
    }
    else if (unit == "w")
    {
        if (valueInUnit > 2)
        {
            durationPerUnit.setText("2");
            return;
        }
        valueInSec = valueInUnit * 7 * 24 * 60 * 60;
    }

    durationInSec.setValue(valueInSec);
    settingDurationPerUnit = durationPerUnit.text();
}
void AdvancedPage::calculatedDurationChanged(const int durationInSec,
                                             IntSetting settingDurationInSec)
{
    settingDurationInSec = durationInSec;
}

void AdvancedPage::timeoutUnitChanged(const QString newUnit,
                                      QStringSetting settingUnit)
{
    settingUnit = newUnit;
}

}  // namespace chatterino
