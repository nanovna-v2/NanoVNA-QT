#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "xavna_mock_ui.H"
namespace Ui {
class xavna_mock_ui_dialog;
}

class xavna_mock_ui_dialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit xavna_mock_ui_dialog(QWidget *parent = 0);
    ~xavna_mock_ui_dialog();
    xavna_ui_changed_cb cb;

private slots:
    void cb_changed();


    void on_slider2_valueChanged(int value);

    void on_slider1_valueChanged(int value);

private:
    Ui::xavna_mock_ui_dialog *ui;
};

#endif // MAINWINDOW_H
