#include "friend.h"
#include "protocol.h"
#include "tcpclient.h"
#include <QInputDialog> //用于输入数据
#include <QDebug>
#include "privatechat.h"
#include <QMessageBox>

//Friend()函数
Friend::Friend(QWidget * parent) : QWidget(parent)
{
    //各种信号槽按钮 点击跳转
    m_pShowMsgTE = new QTextEdit;
    m_pFriendListWidget = new QListWidget;
    m_pInputMsgLE = new QLineEdit;
//    m_pInputMsgLE->setEchoMode(Password);

    m_pDelFriendPB = new QPushButton("删除好友");
    m_pFlushFriendPB = new QPushButton("刷新好友");
    m_pShowOnlineUsrPB = new QPushButton("显示在线用户");
    m_pSearchUsrPB = new QPushButton("查找用户");
    m_pMsgSendPB = new QPushButton("信息发送");
    m_pPrivateChatPB = new QPushButton("私聊"); //m_strCurPath = QString("./%1").arg(m_strLoginName)

    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUsrPB);
    pRightPBVBL->addWidget(m_pSearchUsrPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);


    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    m_pOnline = new Online;

    QVBoxLayout *pMain = new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);
    pMain->addWidget(m_pOnline);
    m_pOnline->hide();

    setLayout(pMain);

    //关联信号槽到槽函数
    connect(m_pShowOnlineUsrPB, SIGNAL(clicked(bool))
            , this, SLOT(showOnline()));
    connect(m_pSearchUsrPB, SIGNAL(clicked(bool))
            , this, SLOT(searchUsr()));
    connect(m_pFlushFriendPB, SIGNAL(clicked(bool))
            , this, SLOT(flushFriend()));
    connect(m_pDelFriendPB, SIGNAL(clicked(bool))
            , this, SLOT(delFriend()));
    connect(m_pPrivateChatPB, SIGNAL(clicked(bool))
            , this, SLOT(privateChat()));
    connect(m_pMsgSendPB, SIGNAL(clicked(bool))
            , this, SLOT(groupChat()));
}

//显示全部在线好友
void Friend::showAllOnlineUsr(PDU *pdu)
{
    if (NULL == pdu)
    {
        return;
    }
    m_pOnline->showUsr(pdu);//显示pdu.caMsg
}

//更新好友列表
void Friend::updateFriendList(PDU *pdu)
{
    if (NULL == pdu)
    {
        return;
    }
    uint uiSize = pdu->uiMsgLen/32; //pdu里总共多少人
    char caName[32] = {'\0'};
    for (uint i=0; i<uiSize; i++)
    {
        memcpy(caName, (char*)(pdu->caMsg)+i*32, 32); //循环拷贝到caName
        m_pFriendListWidget->addItem(caName); //一个一个加到好友列表
    }
}

void Friend::updateGroupMsg(PDU *pdu)
{
    QString strMsg = QString("%1 says: %2").arg(pdu->caData).arg((char*)(pdu->caMsg));
    m_pShowMsgTE->append(strMsg);
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

//显示在线好友
void Friend::showOnline()
{
    if (m_pOnline->isHidden())
    {
        m_pOnline->show(); //直接显示

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST; //设置消息类型并写回
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);//m_tcpSocket.write 写回
        free(pdu);
        pdu = NULL;
    }
    else
    {
        m_pOnline->hide();
    }
}

//槽函数：搜索用户
void Friend::searchUsr()
{
    m_strSearchName = QInputDialog::getText(this, "搜索", "用户名:"); //获得输入的用户名
    if (!m_strSearchName.isEmpty())
    {
        qDebug() << m_strSearchName;
        PDU *pdu = mkPDU(0);
        memcpy(pdu->caData, m_strSearchName.toStdString().c_str(), m_strSearchName.size());//复制到pdu
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //单例 发给服务器去搜索
        free(pdu);
        pdu = NULL;
    }
}

//槽函数：刷新好友列表
void Friend::flushFriend()
{
    m_pFriendListWidget->clear();

    QString strName = TcpClient::getInstance().loginName(); //获得已登录名字
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    memcpy(pdu->caData, strName.toStdString().c_str(), strName.size()); //获得实际数据地址和大小
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //写回服务器
    free(pdu);
    pdu = NULL;
}

//删除好友
void Friend::delFriend()
{
    if (NULL != m_pFriendListWidget->currentItem()) //当前选中行非空
    {
        QString strFriendName = m_pFriendListWidget->currentItem()->text(); //获得当前项目：朋友名字
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
        QString strSelfName = TcpClient::getInstance().loginName(); //找到自己的名字 //c_str获得首地址
        memcpy(pdu->caData, strSelfName.toStdString().c_str(), strSelfName.size()); //前32字节自己名字
        memcpy(pdu->caData+32, strFriendName.toStdString().c_str(), strFriendName.size()); //后32字节朋友名字
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //写回服务器
        free(pdu);
        pdu = NULL;
    }
}

//槽函数：私聊处理
void Friend::privateChat()
{
    if (NULL != m_pFriendListWidget->currentItem()) //当前非空
    {
        QString strChatName = m_pFriendListWidget->currentItem()->text(); //获取私聊的名字
        PrivateChat::getInstance().setChatName(strChatName); //设置私聊的名字
        if (PrivateChat::getInstance().isHidden()) //如果私聊窗口隐藏
        {
            PrivateChat::getInstance().show(); //则显示出来
        }
    }
    else
    {
        QMessageBox::warning(this, "私聊", "请选择私聊对象");
    }
}

//槽函数：群聊处理
void Friend::groupChat()
{
    QString strMsg = m_pInputMsgLE->text(); //获得输入的信息
    if (!strMsg.isEmpty())
    {
        PDU *pdu = mkPDU(strMsg.size()+1); //"\0" ??
        pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
        QString strName = TcpClient::getInstance().loginName(); //单例获取登录名
        strncpy(pdu->caData, strName.toStdString().c_str(), strName.size());//c_str:获取数据首地址
        strncpy((char*)(pdu->caMsg), strMsg.toStdString().c_str(), strMsg.size()); //消息写入
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //写回服务器（pdu需强转为char*）
    }
    else
    {
        QMessageBox::warning(this, "群聊", "信息不能为空");
    }
}
