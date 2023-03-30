#include "privatechat.h"
#include "ui_privatechat.h"
#include "protocol.h"
#include "tcpclient.h"
#include <QMessageBox>

PrivateChat::PrivateChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

PrivateChat &PrivateChat::getInstance() //PrivateChat设为单例模式
{
    static PrivateChat instance;
    return instance;
}

void PrivateChat::setChatName(QString strName) //设置聊天名字
{
    m_strChatName = strName; //设置对方的名字
    m_strLoginName = TcpClient::getInstance().loginName(); //设置我方的名字（单例模式直接通过类名访问）
}

void PrivateChat::updateMsg(const PDU *pdu) //更新窗口的消息
{
    if (NULL == pdu)
    {
        return;
    }
    char caSendName[32] = {'\0'};
    memcpy(caSendName, pdu->caData, 32); //前32字节为发送者
    QString strMsg = QString("%1 says: %2").arg(caSendName).arg((char*)(pdu->caMsg));
    ui->showMsg_te->append(strMsg); //附加到窗口
}

//槽函数：发送按钮点击
void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg = ui->inputMsg_le->text(); //获得输入框的数据
    ui->inputMsg_le->clear(); //清除输入框
    if (!strMsg.isEmpty())  //数据非空
    {
        PDU *pdu = mkPDU(strMsg.size()+1);  //创建一个pdu
        pdu->uiMsgType = ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;

        memcpy(pdu->caData, m_strLoginName.toStdString().c_str(), m_strLoginName.size());
        memcpy(pdu->caData+32, m_strChatName.toStdString().c_str(), m_strChatName.size());
          //c_str()：直接获得数据地址 减少复制  //toStdString()：转化成C++字符串
        strcpy((char*)(pdu->caMsg), strMsg.toStdString().c_str()); //复制聊天信息

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //写回服务器
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::warning(this, "私聊", "发送的聊天信息不能为空");
    }
}
