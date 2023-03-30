#include "mytcpsocket.h"
#include <QDebug>
#include <stdio.h>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>

MyTcpSocket::MyTcpSocket()
{
    connect(this, SIGNAL(readyRead()) //收到客户端的消息，发出readyRead信号，调用recvMsg处理
            , this, SLOT(recvMsg())); //该socket的readyRead信号，用该socket的recvMsg处理
    connect(this, SIGNAL(disconnected())  //自己的信号2 调用自己的函数4
            , this, SLOT(clientOffline())); //客户端下线信号→下线处理槽函数

    m_bUpload = false;
    m_pTimer = new QTimer;
    connect(m_pTimer, SIGNAL(timeout())
            , this, SLOT(sendFileToClient()));
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);

    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();

    QString srcTmp;
    QString destTmp;
    for (int i=0; i<fileInfoList.size(); i++)
    {
        qDebug() << "fileName:" << fileInfoList[i].fileName();
        if (fileInfoList[i].isFile())
        {
            srcTmp = strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp = strDestDir+'/'+fileInfoList[i].fileName();
            QFile::copy(srcTmp, destTmp);
        }
        else if (fileInfoList[i].isDir())
        {
            if (QString(".") == fileInfoList[i].fileName()
                || QString("..") == fileInfoList[i].fileName())
            {
                continue;
            }
            srcTmp = strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp = strDestDir+'/'+fileInfoList[i].fileName();
            copyDir(srcTmp, destTmp);
        }
    }
}

//readyRead信号调用recvMsg 处理客户端发来的数据
void MyTcpSocket::recvMsg()
{
    if (!m_bUpload) //①如果收到数据（上载为0？）
    {
        qDebug() << this->bytesAvailable(); //打印当前可读数据大小
        uint uiPDULen = 0;
        this->read((char*)& uiPDULen, sizeof(uint)); //先读pdu总长
        uint uiMsgLen = uiPDULen - sizeof(PDU); //计算实际消息的长度（pdu总长 减 头部长度）
        PDU *pdu = mkPDU(uiMsgLen);  //新建pdu（扩容复制并返回）
        this->read((char*)pdu+sizeof(uint), uiPDULen-sizeof(uint)); //从1开始 读2那么大
//        char caName[32] = {'\0'};
//        char caPwd[32] = {'\0'};
//        strncpy(caName,pdu->caData,32);
//        strncpy(caPwd,pdu->caData,32);
//        qDebug() << caName << caPwd;
        qDebug() << pdu->uiMsgType ;

        //判断消息类型
        switch (pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_REQUEST: //注册请求
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32); //复制用户名，密码
            strncpy(caPwd, pdu->caData+32, 32);
            bool ret = OpeDB::getInstance().handleRegist(caName, caPwd); //去数据库里面处理
            PDU *respdu = mkPDU(0);  //产生一个反馈pdu
            respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;   //pdu消息类型设为“注册回应”
            if (ret) //数据库操作返回值为真
            {
                strcpy(respdu->caData, REGIST_OK);  //提示词拷贝进pdu
                QDir dir;
                qDebug () << "create dir: " << dir.mkdir(QString("./%1").arg(caName));
            }
            else
            {
                strcpy(respdu->caData, REGIST_FAILED);//修改字段后写回
            }
            write((char*)respdu, respdu->uiPDULen);  //检测缓冲区？类内写回 返回给客户端，客户端收到数据发出readyread信号
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_REQUEST:  //登录请求
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);  //复制，获得用户和密码
            strncpy(caPwd, pdu->caData+32, 32);
            bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);//单例模式调用getInstance 操作数据库匹配，三项符合返回true
            PDU *respdu = mkPDU(0);  //创建反馈pdu
            respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND; //设置消息类型为 “登录请求”
            if (ret)  //数据库验证成功 返回为true
            {
                strcpy(respdu->caData, LOGIN_OK); //把pdu的character data字段，设为ok字段
                m_strName = caName; //登录成功记录名字，方便注销时查询数据库
            }
            else
            {
                strcpy(respdu->caData, LOGIN_FAILED);//修改字段再写回
            }
            write((char*)respdu, respdu->uiPDULen); //写回客户端（从1开始2那么长）
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST: //在线好友请求
        {
            qDebug() << "all online request";
            QStringList ret = OpeDB::getInstance().handleAllOnline(); //调用数据库操作 返回好友列表到ret
            uint uiMsgLen = ret.size()*32; //消息部分长度
            qDebug () << "size=" << ret.size();
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for (int i=0; i<ret.size(); i++) //循环拷贝到pdu
            {
                memcpy((char*)(respdu->caMsg)+i*32 //int转char*  每次偏移32bit
                       , ret.at(i).toStdString().c_str() //用ret列表的at模板 拷贝数据的首地址
                       , ret.at(i).size());  //拷贝数据的大小
                qDebug() << ret.at(i).toStdString().c_str();
                printf("%s\n", (char*)(respdu->caMsg)+i*32);
            }
            write((char*)respdu, respdu->uiPDULen); //修改完，pdu写回客户端
            free(respdu);
            respdu = NULL;

            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST: //搜索用户请求
        {
            int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData); //数据库查询
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if (-1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_NO); //查无此人
            }
            else if (1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_ONLINE); //在线
            }
            else if (0 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_OFFLINE); //不在线
            }
            write((char*)respdu, respdu->uiPDULen); //写回客户端
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: //添加好友请求
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32); //前32位拷贝接受者
            strncpy(caName, pdu->caData+32, 32); //后32位拷贝请求者
            int ret = OpeDB::getInstance().handleAddFriend(caPerName, caName); //数据库查询
            PDU *respdu = NULL;
            if (-1 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, UNKNOW_ERROR); //未知错误
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (0 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, EXISTED_FRIEND); //存在该好友
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (1 == ret) //在线
            {
                MyTcpServer::getInstance().resend(caPerName, pdu); //原样转发给caPerName
            }
            else if (2 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_OFFLINE); //添加好友离线
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (3 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_NO_EXIST); //添加好友不存在
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGGREE: //添加好友同意
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32); //前32位拷贝接受者
            strncpy(caName, pdu->caData+32, 32); //后32位拷贝请求者
            OpeDB::getInstance().handleAgreeAddFriend(caPerName, caName); //数据库添加好友

            MyTcpServer::getInstance().resend(caName, pdu); //转发给请求者客户端
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE: //添加好友拒绝
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData+32, 32);
            MyTcpServer::getInstance().resend(caName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST: //刷新好友请求
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList ret = OpeDB::getInstance().handleFlushFriend(caName); //数据库获取
            uint uiMsgLen = ret.size()*32; //计算实际消息长度
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for (int i=0; i<ret.size(); i++)
            {
                memcpy((char*)(respdu->caMsg)+i*32  //循环拷贝
                       , ret.at(i).toStdString().c_str()
                       , ret.at(i).size());
            }
            write((char*)respdu, respdu->uiPDULen); //写回客户端
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST: //删除好友请求
        {
            char caSelfName[32] = {'\0'};
            char caFriendName[32] = {'\0'};
            strncpy(caSelfName, pdu->caData, 32); //strncpy对char*类型操作  memcpy直接对内存操作，不关乎类型 这里都可以
            strncpy(caFriendName, pdu->caData+32, 32); //前32字节自己名字 后32字节朋友名字
            OpeDB::getInstance().handleDelFriend(caSelfName, caFriendName); //数据库中删除

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData, DEL_FRIEND_OK); //删除成功
            write((char*)respdu, respdu->uiPDULen);//“删除好友回应”写回“删人者”客户端，提示他
            free(respdu);
            respdu = NULL;

            MyTcpServer::getInstance().resend(caFriendName, pdu); //recvMsg接收的pdu“删除好友请求” 转发给“被删者”caFriendName

            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST: //私聊请求
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData+32, 32); //后32字节 接收者名字
            char caName[32] = {'\0'};
            memcpy(caName, pdu->caData, 32);
            qDebug() << caName << "-->" << caPerName << (char*)(pdu->caMsg);
            MyTcpServer::getInstance().resend(caPerName, pdu); //转发给caPerName的客户端

            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST: //群聊请求
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList onlineFriend = OpeDB::getInstance().handleFlushFriend(caName);
            QString tmp;
            for (int i=0; i<onlineFriend.size(); i++)
            {
                tmp = onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmp.toStdString().c_str(), pdu);
            }
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST: //创建目录请求
        {
            QDir dir;
            QString strCurPath = QString("%1").arg((char*)(pdu->caMsg)); //保存pdu中的路径
            qDebug() << strCurPath;
            bool ret = dir.exists(strCurPath); //判断路径是否存在
            PDU *respdu = NULL;
            if (ret)  //当前目录存在
            {
                char caNewDir[32] = {'\0'};
                memcpy(caNewDir, pdu->caData+32, 32);
                QString strNewPath = strCurPath+"/"+caNewDir; //NewDir拼接到新路径上
                qDebug() << strNewPath;
                ret = dir.exists(strNewPath);
                qDebug() << "-->" << ret;
                if (ret)  //创建的文件名已存在
                {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, FILE_NAME_EXIST);
                }
                else   //创建的文件名不存在
                {
                    dir.mkdir(strNewPath);
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, CREAT_DIR_OK);
                }
            }
            else     //当前目录不存在
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData, DIR_NO_EXIST);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST: //刷新文件请求
        {
            char *pCurPath = new char[pdu->uiMsgLen]; //string一开始产生的时候是没有空间的
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);
            QDir dir(pCurPath);
            QFileInfoList fileInfoList = dir.entryInfoList(); //获得文件信息表
            int iFileCount = fileInfoList.size(); //获得文件个数
            PDU *respdu = mkPDU(sizeof(FileInfo)*iFileCount);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
            FileInfo *pFileInfo = NULL; //文件信息结构体
            QString strFileName;
            for (int i=0; i<iFileCount; i++)
            {
                pFileInfo = (FileInfo*)(respdu->caMsg)+i; //+1 则跳过FileInfo这么大
                strFileName = fileInfoList[i].fileName();  //放文件名

                memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                if (fileInfoList[i].isDir()) //是目录
                {
                    pFileInfo->iFileType = 0; //设置文件类型为0
                }
                else if (fileInfoList[i].isFile()) //是文件
                {
                    pFileInfo->iFileType = 1; //设置文件类型为1
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_REQUEST: //删除文件请求
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData); //拷贝到caName
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen); //拷贝到pPath
            QString strPath = QString("%1/%2").arg(pPath).arg(caName); //拼接为完整路径
            qDebug() << strPath;

            QFileInfo fileInfo(strPath); //借助FileInfo
            bool ret = false;
            if (fileInfo.isDir()) //如果是目录
            {
                QDir dir;
                dir.setPath(strPath);
                ret = dir.removeRecursively(); //删除所有内容 ret返回true
            }
            else if (fileInfo.isFile())  //是常规文件
            {
                ret = false;
            }
            PDU *respdu = NULL;
            if (ret)
            {
                respdu = mkPDU(strlen(DEL_DIR_OK)+1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
            }
            else
            {
                respdu = mkPDU(strlen(DEL_DIR_FAILURED)+1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData, DEL_DIR_FAILURED, strlen(DEL_DIR_FAILURED));
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
        {
            char caOldName[32] = {'\0'};
            char caNewName[32] = {'\0'};
            strncpy(caOldName, pdu->caData, 32);
            strncpy(caNewName, pdu->caData+32, 32);

            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);

            qDebug() << strOldPath;
            qDebug() << strNewPath;

            QDir dir;
            bool ret = dir.rename(strOldPath, strNewPath);

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, RENAME_FILE_OK);
            }
            else
            {
                strcpy(respdu->caData, RENAME_FILE_FAILURED);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
        {
            char caEnterName[32] = {'\0'};
            strncpy(caEnterName, pdu->caData, 32);

            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName); //拼接为完整路径

    //        qDebug() << strPath;

            QFileInfo fileInfo(strPath); //用fileInfo判断是目录还是文件
            PDU *respdu = NULL;
            if (fileInfo.isDir()) //为目录
            {
                QDir dir(strPath);
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount = fileInfoList.size();
                respdu = mkPDU(sizeof(FileInfo)*iFileCount);
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
                FileInfo *pFileInfo = NULL;
                QString strFileName;
                for (int i=0; i<iFileCount; i++)
                {
                    pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                    strFileName = fileInfoList[i].fileName();

                    memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                    if (fileInfoList[i].isDir())
                    {
                        pFileInfo->iFileType = 0;
                    }
                    else if (fileInfoList[i].isFile())
                    {
                        pFileInfo->iFileType = 1;
                    }
                }
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (fileInfo.isFile()) //为文件
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strcpy(respdu->caData, ENTER_DIR_FAILURED); //进入失败

                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }

            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);
            qDebug() << strPath;

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if (fileInfo.isDir())
            {
                ret = false;
            }
            else if (fileInfo.isFile())  //常规文件
            {
                QDir dir;
                ret = dir.remove(strPath);
            }
            PDU *respdu = NULL;
            if (ret)
            {
                respdu = mkPDU(strlen(DEL_FILE_OK)+1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData, DEL_FILE_OK, strlen(DEL_FILE_OK));
            }
            else
            {
                respdu = mkPDU(strlen(DEL_FILE_FAILURED)+1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData, DEL_FILE_FAILURED, strlen(DEL_FILE_FAILURED));
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug() << strPath;
            delete []pPath;
            pPath = NULL;

            m_file.setFileName(strPath);
            //以只写的方式打开文件，若文件不存在，则会自动创建文件
            if (m_file.open(QIODevice::WriteOnly))
            {
                m_bUpload = true;
                m_iTotal = fileSize;
                m_iRecved = 0;
            }

            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            strcpy(caFileName, pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug() << strPath;
            delete []pPath;
            pPath = NULL;

            QFileInfo fileInfo(strPath);
            qint64 fileSize = fileInfo.size();
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            sprintf(respdu->caData, "%s %lld", caFileName, fileSize);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            m_file.setFileName(strPath);
            m_file.open(QIODevice::ReadOnly);
            m_pTimer->start(1000);

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        {
            char caSendName[32] = {'\0'};
            int num = 0;
            sscanf(pdu->caData, "%s%d", caSendName, &num);
            int size = num*32;
            PDU *respdu = mkPDU(pdu->uiMsgLen-size);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strcpy(respdu->caData, caSendName);
            memcpy(respdu->caMsg, (char*)(pdu->caMsg)+size, pdu->uiMsgLen-size);

            char caRecvName[32] = {'\0'};
            for (int i=0; i<num; i++)
            {
                memcpy(caRecvName, (char*)(pdu->caMsg)+i*32, 32);
                MyTcpServer::getInstance().resend(caRecvName, respdu);
            }
            free(respdu);
            respdu = NULL;

            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(respdu->caData, "share file ok");
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND:
        {
            QString strRecvPath = QString("./%1").arg(pdu->caData);
            QString strShareFilePath = QString("%1").arg((char*)(pdu->caMsg));
            int index = strShareFilePath.lastIndexOf('/');
            QString strFileName = strShareFilePath.right(strShareFilePath.size()-index-1);
            strRecvPath = strRecvPath+'/'+strFileName;

            QFileInfo fileInfo(strShareFilePath);
            if (fileInfo.isFile())
            {
                QFile::copy(strShareFilePath, strRecvPath);
            }
            else if (fileInfo.isDir())
            {
                copyDir(strShareFilePath, strRecvPath);
            }
            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            int srcLen = 0;
            int destLen = 0;
            sscanf(pdu->caData, "%d%d%s", &srcLen, &destLen, caFileName);

            char *pSrcPath = new char[srcLen+1];
            char *pDestPath = new char[destLen+1+32];
            memset(pSrcPath, '\0', srcLen+1);
            memset(pDestPath, '\0', destLen+1+32);

            memcpy(pSrcPath, pdu->caMsg, srcLen);
            memcpy(pDestPath, (char*)(pdu->caMsg)+(srcLen+1), destLen);

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
            QFileInfo fileInfo(pDestPath);
            if (fileInfo.isDir())
            {
                strcat(pDestPath, "/");
                strcat(pDestPath, caFileName);

                bool ret = QFile::rename(pSrcPath, pDestPath);
                if (ret)
                {
                    strcpy(respdu->caData, MOVE_FILE_OK);
                }
                else
                {
                    strcpy(respdu->caData, COMMON_ERR);
                }
            }
            else if (fileInfo.isFile())
            {
                strcpy(respdu->caData, MOVE_FILE_FAILURED);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }
        default:
            break;
        }

        free(pdu);
        pdu = NULL;
    }
    else
    {
        PDU *respdu = NULL;
        respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;

        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();
        if (m_iTotal == m_iRecved)
        {
            m_file.close();
            m_bUpload = false;

            strcpy(respdu->caData, UPLOAD_FILE_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (m_iTotal < m_iRecved)
        {
            m_file.close();
            m_bUpload = false;
            strcpy(respdu->caData, UPLOAD_FILE_FAILURED);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }

    }
    //    qDebug() << caName << caPwd << pdu->uiMsgType;
}

//槽函数：客户端下线处理（关联disconnected信号）
void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str()); //单例模式操作数据库处理
    emit offline(this); //修改在线状态后，发出offline信号调用deleteSocket
}

void MyTcpSocket::sendFileToClient()
{
    char *pData = new char[4096];
    qint64 ret = 0;
    while (true)
    {
        ret = m_file.read(pData, 4096);
        if (ret > 0 && ret <= 4096)
        {
            write(pData, ret);
        }
        else if (0 == ret)
        {
            m_file.close();
            break;
        }
        else if (ret < 0)
        {
            qDebug() << "发送文件内容给客户端过程中失败";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData = NULL;
}
