#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "timer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Timer *timer;
    QTimer *_timer;

private slots:
    void closeEvent(QCloseEvent *event) override;

    void show_timer();
    void render_qr();
    void show_login();

    void on_tick();
    void on_timerButton_clicked();

    void on_buttonAuthLogout_clicked();
    void on_buttonAuthCreate_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
