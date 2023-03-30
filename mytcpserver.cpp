#include "mytcpserver.h"
#include <QDebug>

MyTcpServer::MyTcpServer()
{

}

//用getInstance调用MyTcpServer单例
MyTcpServer &MyTcpServer::getInstance()  //单例模式：通过类名调用getInstance
{
    static MyTcpServer instance;
    return instance;  //返回静态的局部对象
}

//listen自动调用incomingConnection（参数为listen返回的描述符）
void MyTcpServer::incomingConnection(qintptr socketDescriptor)//对虚函数incomingConnection重定义
{
    qDebug() << "new client connected"; //提示客户端连接成功
    MyTcpSocket *pTcpSocket = new MyTcpSocket;  //产生一个socket
    pTcpSocket->setSocketDescriptor(socketDescriptor); //listen返回的描述符保存到socket，即可与客户端通信
    m_tcpSocketList.append(pTcpSocket); //产生的socket添加到socket链表里（客户端下线要删除 节省资源）

    connect(pTcpSocket, SIGNAL(offline(MyTcpSocket*)) //关联下线信号到“删除socket函数”
            , this, SLOT(deleteSocket(MyTcpSocket*)));
}

//转发
void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if (NULL == pername || NULL == pdu)
    {
        return;
    }
    QString strName = pername;
    for (int i=0; i<m_tcpSocketList.size(); i++) //遍历链表
    {
        if (strName == m_tcpSocketList.at(i)->getName()) //找到该用户
        {
            m_tcpSocketList.at(i)->write((char*)pdu, pdu->uiPDULen); //原样转发给pername
            break;
        }
    }
}

//删除列表中的socket
void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    QList<MyTcpSocket*>::iterator iter = m_tcpSocketList.begin(); //list迭代器指向第一项
    for (; iter != m_tcpSocketList.end(); iter++) //list迭代器遍历列表
    {
        if (mysocket == *iter) //如果指向对应的socket对象
        {
//            delete *iter;
//            *iter = NULL;
            m_tcpSocketList.erase(iter); //删除socket
            break;
        }
    }
    for (int i=0; i<m_tcpSocketList.size(); i++)
    {
        qDebug() << m_tcpSocketList.at(i)->getName(); //用公共方法getName，遍历打印列表
    }
}




