#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


namespace Ui {
class MainWindow;
}

class MainWindowPrivate;
class MainWindow : public QMainWindow
{
    Q_OBJECT

    MainWindowPrivate *d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_acceptButton_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
