#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H

#include <QWidget>
#include "protocol.h"

namespace Ui {
class PrivateChat;
}

class PrivateChat : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateChat(QWidget *parent = 0);
    ~PrivateChat();

    static PrivateChat &getInstance();

    void setChatName(QString strName); //设置聊天对象
    void updateMsg(const PDU *pdu);  //更新窗口的消息

private slots:
    void on_sendMsg_pb_clicked(); //槽函数：发送按钮点击

private:
    Ui::PrivateChat *ui;
    QString m_strChatName; //保存聊天的对象的名字
    QString m_strLoginName;
};

#endif // PRIVATECHAT_H
