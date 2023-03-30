#include "online.h"
#include "ui_online.h"
#include <QDebug>
#include "tcpclient.h"

Online::Online(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online:: ~Online()
{
    delete ui;
}

//槽函数：搜索用户
void Online::showUsr(PDU *pdu) //显示好友用户pdu.caMsg
{
    if (NULL == pdu)
    {
        return;
    }
    uint uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    for (uint i=0; i<uiSize; i++)
    {
        memcpy(caTmp, (char*)(pdu->caMsg)+i*32, 32); //拷贝
        qDebug() << caTmp;
        ui->online_lw->addItem(caTmp); //显示
    }
}

//槽函数：加好友
void Online::on_addFriend_pb_clicked()
{
    QListWidgetItem *pItem = ui->online_lw->currentItem(); //online_lw: less widget
    QString strPerUsrName = pItem->text(); //获得用户名
    QString strLoginName = TcpClient::getInstance().loginName();//获得登录名（私有属性用公共接口获取）
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData, strPerUsrName.toStdString().c_str(), strPerUsrName.size()); //前32位拷贝接收者用户名
    memcpy(pdu->caData+32, strLoginName.toStdString().c_str(), strLoginName.size());//后32位拷贝请求者登录名
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen); //把请求 写回服务器
    free(pdu);
    pdu = NULL;
}

void Online::on_LoginName_highlighted(const QString &arg1)
{

}
