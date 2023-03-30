#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include "protocol.h"
#include "opewidget.h"

namespace Ui {
class TcpClient;
}

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = 0);
    ~TcpClient();
    void loadConfig();  //加载配置文件（在构造函数调用）

    //单例模式：设置静态的成员函数，函数产生一个静态的对象，每次调用都是同一实例
    static TcpClient & getInstance(); //TcpClient也设为单例模式，通过类名调用，想在哪调用就在哪调用
    QTcpSocket &getTcpSocket();
    QString loginName();
    QString curPath();
    void setCurPath(QString strCurPath);

public slots:
    void showConnect();//添加槽函数处理信号 提示连接成败
    void recvMsg(); //接收tcpsocket的数据

private slots:
//    void on_send_pb_clicked();

    void on_login_pb_clicked();

    void on_regist_pb_clicked();

    void on_cancel_pb_clicked();

private:
    Ui::TcpClient *ui;
    QString m_strIP;  //全局变量存放读取的IP
    quint16 m_usPort;  //无符号16位短整形的端口 就能存放8888

    //连接服务器，和服务器数据交互
    QTcpSocket m_tcpSocket;  //函数有默认值，在头文件直接作为 成员变量
    QString m_strLoginName;

    QString m_strCurPath; //记录当前路径
    QFile m_file;
};

#endif // TCPCLIENT_H
