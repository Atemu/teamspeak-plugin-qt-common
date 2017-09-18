#pragma once

#include <memory>
#include <QtWidgets/QWidget>

namespace Ui
{
class FaderVertical;
}

class FaderVertical : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float value
    READ getValue
    WRITE setValue
    NOTIFY valueChanged)
    
public:
    explicit FaderVertical(QWidget *parent = 0);
    ~ FaderVertical();

    float getValue() const;
    void setValue(float val) { onDoubleSpinBoxValueChanged(static_cast< float >(val)); }

signals:
    void valueChanged(float);

private slots:
    void onSliderValueChanged(int);
    void onDoubleSpinBoxValueChanged(double);

private:
    std::unique_ptr<Ui::FaderVertical> ui;
};
