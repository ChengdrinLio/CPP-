#include "tcpserver.h"
#include "ui_tcpserver.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include <QFile>

TcpServer::TcpServer(QWidget *parent) :QWidget(parent),ui(new Ui::TcpServer)
{
    ui->setupUi(this);

    loadConfig();  //①加载配置文件（同tcpclient）
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP), m_usPort); //②用TcpServer监听 返回文件描述符
    //成功连接，产生readyRead信号，自动调用incomingConnection
    //每连接一个客户端，都产生一个新的socket，
    //描述符返回到incomingConnection，用TcpSocket保存，与客户端通信
}

TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::loadConfig()
{
    QFile file(":/server.config");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray baData = file.readAll();
        QString strData = baData.toStdString().c_str();
        file.close();

        strData.replace("\r\n", " ");

        QStringList strList = strData.split(" ");

        m_strIP = strList.at(0);
        m_usPort = strList.at(1).toUShort();
        qDebug() << "ip:" << m_strIP << " port:" << m_usPort;
    }
    else
    {
        QMessageBox::critical(this, "open config", "open config failed");
    }
}
