#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mhserver.h"
#include <memory>
#include <QNetworkInterface>
#include <QDebug>

class MainWindowPrivate
{
public:
    MainWindow *const q_ptr;
    Q_DECLARE_PUBLIC(MainWindow)

    MHServer * server;

    MainWindowPrivate(MainWindow *parent):
    q_ptr(parent)
    {

    }

    void startServer()
    {
        Q_Q(MainWindow);
        server = new MHServer(q);
        server->listen();
    }

    QString getLocalIp()
    {
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
                 return address.toString();
        }
        return "localhost";
    }

    QString getServerAddress()
    {
        int port = server->serverPort();
        return QString("http://%1:%2/").arg(getLocalIp(), QString::number(port));
    }

    void acceptClient(QString passcode) {
        server->acceptPasscode(passcode);
    }

};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d_ptr(new MainWindowPrivate(this)),
    ui(new Ui::MainWindow)
{
    Q_D(MainWindow);
    ui->setupUi(this);

    d->startServer();

    ui->labelAddress->setText(d->getServerAddress());

    setMaximumSize(this->size());
    setMinimumSize(this->size());
    this->statusBar()->setSizeGripEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_acceptButton_clicked()
{
    Q_D(MainWindow);
    d->acceptClient(ui->passcodeEdit->text());
    ui->passcodeEdit->setText("");
}
