#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include "opedb.h"
#include <QDir> //qt 目录操作的类
#include <QFile>
#include <QTimer>

class MyTcpSocket : public QTcpSocket //四重继承到QObject，再加Q_OBJECT宏，该类就支持信号槽
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString getName();
    void copyDir(QString strSrcDir, QString strDestDir);

signals:
    void offline(MyTcpSocket *mysocket); //下线信号，用于删除链表里的socket

public slots:
    void recvMsg(); //该socket的readyRead信号，用该socket的recvMsg处理
    void clientOffline(); //客户端下线处理函数声明
    void sendFileToClient();

private:
    QString m_strName; //下线时修改数据库，需要用名字查询

    QFile m_file;
    qint64 m_iTotal;
    qint64 m_iRecved;
    bool m_bUpload;

    QTimer *m_pTimer;
};

#endif // MYTCPSOCKET_H
