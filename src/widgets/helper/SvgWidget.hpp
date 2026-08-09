#pragma once

#include <QWidget>

class QSvgRenderer;

namespace chatterino {

class SvgWidget : public QWidget
{
public:
    SvgWidget(QWidget *parent = nullptr);

    QSvgRenderer *renderer();

    QSize sizeHint() const override;

    void load(const QString &file);
    void load(const QByteArray &contents);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSvgRenderer *renderer_;
};

}  // namespace chatterino
