#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include "privatechat.h"

TcpClient::TcpClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpClient)//显示并关联信号
{
    ui->setupUi(this);

    resize(600, 300);

    loadConfig();

    //关联connected信号（发送者，信号类型, 接受者，信号处理函数）
    connect(&m_tcpSocket, SIGNAL(connected())//connected信号 关联showConnect函数
            , this, SLOT(showConnect()));
    connect(&m_tcpSocket, SIGNAL(readyRead()) //客户端收到数据就发出readyRead信号
            , this, SLOT(recvMsg())); //关联信号槽，调用recvMsg()接收


    m_tcpSocket.connectToHost(QHostAddress(m_strIP), m_usPort);//通过socket 既用来连接服务器，又用来收发数据
    //成功连接服务器 则发出connected信号
}

TcpClient::~TcpClient()
{
    delete ui;
}


void TcpClient::loadConfig()//配置文件加载过程
{
    QFile file(":/client.config");  //产生file对象（传入代码路径，冒号 前缀 文件名）
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray baData = file.readAll();  //读取
        QString strData = baData.toStdString().c_str();  //转换成字符串（字节类型 → char* → 字符串）
        file.close();  //文件用完 关闭

        strData.replace("\r\n", " ");  //回车换行 换成 空格

        QStringList strList = strData.split(" ");  //切分并返回字符串列表

        m_strIP = strList.at(0);  //服务器的IP和端口保存到成员变量
        m_usPort = strList.at(1).toUShort();  //字符串型要转化成无符号整形
        qDebug() << "ip:" << m_strIP << " port:" << m_usPort;  //打印测试
    }
    else
    {
        QMessageBox::critical(this, "open config", "open config failed");  //读取失败提示
    }
}

TcpClient &TcpClient::getInstance() //每次调用返回的都是同一个指针常量（同一个实例，引用即指针常量）
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()  //单例模式：返回m_tcpSocket
{
    return m_tcpSocket;
}

QString TcpClient::loginName() //用公共接口访问私有属性
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

void TcpClient::setCurPath(QString strCurPath)
{
    m_strCurPath = strCurPath;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this, "连接服务器", "连接服务器成功");  //MessageBox消息反馈
}

//readyRead信号调用recvMsg 接收消息处理
void TcpClient::recvMsg()
{
    if (!OpeWidget::getInstance().getBook()->getDownloadStatus())
    {
        qDebug() << m_tcpSocket.bytesAvailable();  //打印数据大小
        uint uiPDULen = 0;
        m_tcpSocket.read((char*)&uiPDULen, sizeof(uint)); //读前面的数据
        uint uiMsgLen = uiPDULen-sizeof(PDU); //计算实际消息长度
        PDU *pdu = mkPDU(uiMsgLen);
        m_tcpSocket.read((char*)pdu+sizeof(uint), uiPDULen-sizeof(uint));//读后面的数据
        switch (pdu->uiMsgType) //判断消息类型
        {
        case ENUM_MSG_TYPE_REGIST_RESPOND: //注册回应
        {
            if (0 == strcmp(pdu->caData, REGIST_OK)) //pdu字段为成功
            {
                QMessageBox::information(this, "注册", REGIST_OK);
            }
            else if (0 == strcmp(pdu->caData, REGIST_FAILED)) //pdu字段为失败
            {
                QMessageBox::warning(this, "注册", REGIST_FAILED);
            }
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_RESPOND: //登录回应
        {
            if (0 == strcmp(pdu->caData, LOGIN_OK)) //如果pdu字段为ok时
            {
                m_strCurPath = QString("./%1").arg(m_strLoginName); //用户名作为当前目录
                QMessageBox::information(this, "登录", LOGIN_OK); //给出消息提示
                //显示好友窗口
                OpeWidget::getInstance().show();  //通过调用静态函数getInstance，获得静态对象的引用，把好友窗口显示出来；不能直接创建在栈区 函数结束会销毁
                hide(); //隐藏登录界面
            }
            else if (0 == strcmp(pdu->caData, LOGIN_FAILED))
            {
                QMessageBox::warning(this, "登录", LOGIN_FAILED);
            }
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND: //全部在线回应
        {
            OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu); //私有属性m_pFriend指针调用show..
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_RESPOND: //搜索用户回应
        {
            if (0 == strcmp(SEARCH_USR_NO, pdu->caData)) //pdu字段为不存在：m_strSearchName not exist
            {
                QMessageBox::information(this, "搜索", QString("%1: not exist").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            else if (0 == strcmp(SEARCH_USR_ONLINE, pdu->caData))//pdu字段为在线：m_strSearchName online
            {
                QMessageBox::information(this, "搜索", QString("%1: online").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            else if (0 == strcmp(SEARCH_USR_OFFLINE, pdu->caData))//pdu字段为不存在：m_strSearchName offline
            {
                QMessageBox::information(this, "搜索", QString("%1: offline").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: //添加好友请求
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData+32, 32); //复制后32位请求者
            //信息对话框
            int ret = QMessageBox::information(this, "添加好友", QString("%1 want to add you as friend ?").arg(caName)
                                               , QMessageBox::Yes, QMessageBox::No); //QString.arg 用caName依次替换%1
            PDU *respdu = mkPDU(0);
            memcpy(respdu->caData, pdu->caData, 64); //复制接收者和请求者
            if (QMessageBox::Yes == ret)
            {
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGGREE; //添加好友同意
            }
            else
            {
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
            }
            m_tcpSocket.write((char*)respdu, respdu->uiPDULen); //写回服务器
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND: //添加好友回应
        {
            QMessageBox::information(this, "添加好友", pdu->caData);
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGGREE: //添加好友同意
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData, 32); //前32位拷贝接受者
            QMessageBox::information(this, "添加好友", QString("添加%1好友成功").arg(caPerName));
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE: //添加好友拒绝
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData, 32);
            QMessageBox::information(this, "添加好友", QString("添加%1好友失败").arg(caPerName));
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND: //刷新好友回应
        {
            OpeWidget::getInstance().getFriend()->updateFriendList(pdu); //单例+getFriend 传到窗口并更新
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST: //删除好友请求：%1删除你作为他的好友
        {
            char caName[32] = {'\0'};
            memcpy(caName, pdu->caData, 32);
            QMessageBox::information(this, "删除好友", QString("%1 删除你作为他的好友").arg(caName));
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND: //删除好友回应：删除好友成功
        {
            QMessageBox::information(this, "删除好友", "删除好友成功");
            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST: //私聊请求
        {
            if (PrivateChat::getInstance().isHidden()) //如果聊天窗口隐藏 则显示
            {
                PrivateChat::getInstance().show();
            }
            char caSendName[32] = {'\0'};
            memcpy(caSendName, pdu->caData, 32);
            QString strSendName = caSendName;
            PrivateChat::getInstance().setChatName(strSendName); //设置发送者的姓名

            PrivateChat::getInstance().updateMsg(pdu); //更新窗口的消息

            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            OpeWidget::getInstance().getFriend()->updateGroupMsg(pdu);
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_RESPOND: //创建文件夹请求
        {
            QMessageBox::information(this, "创建文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND: //刷新文件请求
        {
            OpeWidget::getInstance().getBook()->updateFileList(pdu);
            QString strEnterDir = OpeWidget::getInstance().getBook()->enterDir();
            if (!strEnterDir.isEmpty())
            {
                m_strCurPath = m_strCurPath+"/"+strEnterDir; //拼接
                qDebug() << "enter dir:" << m_strCurPath;
            }
            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_RESPOND:
        {
            QMessageBox::information(this, "删除文件夹", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_RESPOND:
        {
            QMessageBox::information(this, "重命名文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_RESPOND:
        {
            OpeWidget::getInstance().getBook()->clearEnterDir(); //清除原目录
            QMessageBox::information(this, "进入文件夹", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_RESPOND:
        {
            QMessageBox::information(this, "删除文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:
        {
            QMessageBox::information(this, "上传文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:
        {
            qDebug() << pdu->caData;
            char caFileName[32] = {'\0'};
            sscanf(pdu->caData, "%s %lld", caFileName, &(OpeWidget::getInstance().getBook()->m_iTotal));
            if (strlen(caFileName) > 0 && OpeWidget::getInstance().getBook()->m_iTotal > 0)
            {
                OpeWidget::getInstance().getBook()->setDownloadStatus(true);
                m_file.setFileName(OpeWidget::getInstance().getBook()->getSaveFilePath());
                if (!m_file.open(QIODevice::WriteOnly))
                {
                    QMessageBox::warning(this, "下载文件", "获得保存文件的路径失败");
                }
            }
            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:
        {
            QMessageBox::information(this, "共享文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE:
        {
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            // //aa//bb/cc
            char *pos = strrchr(pPath, '/');
            if (NULL != pos)
            {
                pos++;
                QString strNote = QString("%1 share file->%2 \n Do you accept ?").arg(pdu->caData).arg(pos);
                int ret = QMessageBox::question(this, "共享文件", strNote);
                if (QMessageBox::Yes == ret)
                {
                    PDU *respdu = mkPDU(pdu->uiMsgLen);
                    respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND;
                    memcpy(respdu->caMsg, pdu->caMsg, pdu->uiMsgLen);
                    QString strName = TcpClient::getInstance().loginName();
                    strcpy(respdu->caData, strName.toStdString().c_str());
                    m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
                }
            }
            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_RESPOND:
        {
            QMessageBox::information(this, "移动文件", pdu->caData);
        }
        default:
            break;
        }

        free(pdu);
        pdu = NULL;
    }
    else
    {
        QByteArray buffer = m_tcpSocket.readAll();
        m_file.write(buffer);
        Book *pBook = OpeWidget::getInstance().getBook();
        pBook->m_iRecved += buffer.size();
        if (pBook->m_iTotal == pBook->m_iRecved)
        {
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::information(this, "下载文件", "下载文件成功");
        }
        else if (pBook->m_iTotal < pBook->m_iRecved)
        {
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);

            QMessageBox::critical(this, "下载文件", "下载文件失败");
        }
    }
}
#if 0
void TcpClient::on_send_pb_clicked()
{
    QString strMsg = ui->lineEdit->text();
    if (!strMsg.isEmpty())
    {
        PDU *pdu = mkPDU(strMsg.size()+1);
        pdu->uiMsgType = 8888;
        memcpy(pdu->caMsg, strMsg.toStdString().c_str(), strMsg.size());
        qDebug() << (char*)(pdu->caMsg);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::warning(this, "信息发送", "发送的信息不能为空");
    }
}
#endif

//槽函数：登录
void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text(); //从界面读取数据
    QString strPwd = ui->pwd_le->text();
    if (!strName.isEmpty() && !strPwd.isEmpty()) //非空
    {
        m_strLoginName = strName; //登录名保存到TcpClient的私有成员中
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
        strncpy(pdu->caData, strName.toStdString().c_str(), 32);  //pdu数组前32位放用户名
        strncpy(pdu->caData+32, strPwd.toStdString().c_str(), 32);  //pdu数组后32位放密码
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen); //m_tcpSocket实例.写回给服务器
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::critical(this, "登录", "登录失败:用户名或者密码为空");
    }
}

//槽函数：注册
void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text(); //获得用户名
    QString strPwd = ui->pwd_le->text();  //获得密码
    if (!strName.isEmpty() && !strPwd.isEmpty()) //如果非空
    {
        PDU *pdu = mkPDU(0); //只用到pdu前面的字段
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST; //写入消息类型
        strncpy(pdu->caData, strName.toStdString().c_str(), 32); //pdu前32字节存用户名
        strncpy(pdu->caData+32, strPwd.toStdString().c_str(), 32); //pdu后32字节存密码
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen); //通过m_tcpSocket写回客户端
        free(pdu); //释放pdu
        pdu = NULL;
    }
    else
    {
        QMessageBox::critical(this, "注册", "注册失败:用户名或者密码为空");
    }
}

void TcpClient::on_cancel_pb_clicked()
{

}
