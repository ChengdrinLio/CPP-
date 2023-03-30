#ifndef FRIEND_H
#define FRIEND_H

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "online.h"

class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = 0);
    void showAllOnlineUsr(PDU *pdu); //显示全部在线好友
    void updateFriendList(PDU *pdu);
    void updateGroupMsg(PDU *pdu);

    QString m_strSearchName;

    QListWidget *getFriendList();

//signals:

public slots: //槽函数
    void showOnline();
    void searchUsr();
    void flushFriend(); //刷新好友列表
    void delFriend();  //删除好友
    void privateChat(); //私聊处理
    void groupChat();  //聊处理

private:
    QTextEdit *m_pShowMsgTE;
    QListWidget *m_pFriendListWidget;
    QLineEdit *m_pInputMsgLE;

    QPushButton *m_pDelFriendPB;
    QPushButton *m_pFlushFriendPB;
    QPushButton *m_pShowOnlineUsrPB;
    QPushButton *m_pSearchUsrPB;
    QPushButton *m_pMsgSendPB;
    QPushButton *m_pPrivateChatPB; //私聊按钮

    Online *m_pOnline;


};

#endif // FRIEND_H
