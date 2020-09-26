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
