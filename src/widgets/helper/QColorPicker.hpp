#pragma once

#include <QSpinBox>

namespace chatterino {

// These classes are literally copied from the Qt source
class QColorPicker : public QFrame
{
    Q_OBJECT
public:
    QColorPicker(QWidget *parent);
    ~QColorPicker();
    void setCrossVisible(bool visible);

public slots:
    void setCol(int h, int s);

signals:
    void newCol(int h, int s);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    int hue;
    int sat;
    QPoint colPt();
    int huePt(const QPoint &pt);
    int satPt(const QPoint &pt);
    void setCol(const QPoint &pt);
    QPixmap pix;
    bool crossVisible;
};

static const int COLOR_PICKER_WIDTH = 220;
static const int COLOR_PICKER_HEIGHT = 200;

class QColorLuminancePicker : public QWidget
{
    Q_OBJECT
public:
    QColorLuminancePicker(QWidget *parent = 0);
    ~QColorLuminancePicker();

public slots:
    void setCol(int h, int s, int v);
    void setCol(int h, int s);

signals:
    void newHsv(int h, int s, int v);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    enum { foff = 3, coff = 4 };  //frame and contents offset
    int val;
    int hue;
    int sat;
    int y2val(int y);
    int val2y(int val);
    void setVal(int v);
    QPixmap *pix;
};

static const int LUMINANCE_PICKER_WIDTH = 25;
static const int LUMINANCE_PICKER_HEIGHT = COLOR_PICKER_HEIGHT;

class QColSpinBox : public QSpinBox
{
public:
    QColSpinBox(QWidget *parent);

    void setValue(int i);
};

}  // namespace chatterino
