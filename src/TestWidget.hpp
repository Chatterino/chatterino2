#pragma once

#include <QByteArray>
#include <QItemEditorFactory>
#include <QObject>
#include <QWidget>

namespace chatterino {

class TestWidgetImpl;

class TestWidget : public QWidget
{
    Q_OBJECT

public:
    TestWidget(QWidget *parent);

    void update(const QString &data);

private:
    TestWidgetImpl *impl;
};

class TestWidgetCreator : public QItemEditorFactory
{
public:
    QWidget *createEditor(int userType, QWidget *parent) const override
    {
        qInfo() << "XXX: USER TYPE" << userType;
        return new TestWidget(parent);
    }

    QByteArray valuePropertyName(int userType) const override
    {
        return "data";
    }
};

}  // namespace chatterino

Q_DECLARE_METATYPE(chatterino::TestWidget *);
