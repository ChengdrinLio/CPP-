#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include <QList> //保存所有socket
#include "mytcpsocket.h"

class MyTcpServer : public QTcpServer  //继承QTcpServer为派生类
{
    Q_OBJECT  //支持信号槽
public:
    MyTcpServer();
    static MyTcpServer &getInstance();  //用静态成员函数实现单例模式

    //TCPserver监听后，一旦有客户端连接自动调用incomingConnection处理，并提示
    void incomingConnection(qintptr socketDescriptor); //对虚函数incomingConnection重定义

    void resend(const char *pername, PDU *pdu);

public slots:
    void deleteSocket(MyTcpSocket *mysocket);

private:
    QList<MyTcpSocket*> m_tcpSocketList; // list容器定义一个链表 保存所有socket

};

#endif // MYTCPSERVER_H
