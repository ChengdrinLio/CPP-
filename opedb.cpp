#include "opedb.h"
#include <QMessageBox>
#include <QDebug>

OpeDB::OpeDB(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE"); //通过m_db 告诉qt操作的数据库为Sqlite
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance; //在静态的成员函数里面定义一个静态的对象
    return instance;  //返回这个静态对象的引用
}

//初始化 连接数据库
void OpeDB::init()
{
    m_db.setHostName("localhost"); //本地数据库
    m_db.setDatabaseName("D:\\NetworkDisk System\\TcpServer\\cloud.db");
    if (m_db.open())
    {
        QSqlQuery query;
        query.exec("select * from usrInfo"); //查询语句
        while (query.next())
        {
            QString data = QString("%1,%2,%3").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
            qDebug() << data; //连接成功 直接打印数据
        }
    }
    else
    {
        QMessageBox::critical(NULL, "打开数据库", "打开数据库失败"); //严重警告错误对话框
    }
}

OpeDB::~OpeDB()
{
    m_db.close(); //析构，关闭数据库
}

//数据库操作：注册写入
bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd)
    {
        qDebug() << "name | pwd is NULL";
        return false;
    }
    //写入数据库
    QString data = QString("insert into usrInfo(name, pwd) values(\'%1\',\'%2\')").arg(name).arg(pwd);
    qDebug() << data;
    QSqlQuery query;
    return query.exec(data);
}

//数据库操作：登录验证
bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd) //如果姓名密码非空，直接打印
    {
        qDebug() << "name | pwd is NULL";
        return false;
    }
    //查询数据库
    QString data = QString("select * from usrInfo where name=\'%1\' and pwd = \'%2\' and online=0").arg(name).arg(pwd);
    qDebug() << data; //data接收到了符合三条件的数据，并打印
    QSqlQuery query;
    query.exec(data); //获得相应结果集
    if (query.next()) //如果登录成功 （next获得了数据，并返回true）
    {
        //将库中online状态设为1，并返回true
        data = QString("update usrInfo set online=1 where name=\'%1\' and pwd = \'%2\'").arg(name).arg(pwd);
        qDebug() << data;
        QSqlQuery query;
        query.exec(data);

        return true;
    }
    else
    {
        return false;
    }
}

//数据库操作：下线处理
void OpeDB::handleOffline(const char *name)
{
    if (NULL == name)
    {
        qDebug() << "name is NULL";
        return;
    }
    QString data = QString("update usrInfo set online=0 where name=\'%1\'").arg(name);
//    QString("select * from usrInfo;");
    QSqlQuery query;
    query.exec(data);
}

//数据库操作：全部在线好友
QStringList OpeDB::handleAllOnline()
{
    QString data = QString("select name from usrInfo where online=1");

    qDebug() << "db___" << data;
    QSqlQuery query;
    query.exec(data);
    QStringList result;
    result.clear();

    while (query.next()) //遍历查询
    {
        result.append(query.value(0).toString());  //value获得数据并转化成字符串 并添加到result结果集
        qDebug() << query.value(0).toString();
    }
    qDebug () << "-------------------";
    return result; //返回结果集result
}

//数据库操作：搜索好友
int OpeDB::handleSearchUsr(const char *name)
{
    if (NULL == name)
    {
        return -1;
    }
    QString data = QString("select online from usrInfo where name=\'%1\'").arg(name);//把名字通过替换的形式拼装成一个语句
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        int ret = query.value(0).toInt();
        if (1 == ret) //在线
        {
            return 1;
        }
        else if (0 == ret) //不在线
        {
            return 0;
        }
    }
    else
    {
        return -1; //查无此人
    }
}

//数据库操作：添加好友
int OpeDB::handleAddFriend(const char *pername, const char *name)
{
    if (NULL == pername || NULL == name) //名字错误
    {
        return -1;
    }
    //联表查询 互相包含
    QString data = QString("select * from friend where (id=(select id from usrInfo where name=\'%1\') and friendId = (select id from usrInfo where name=\'%2\')) "
                           "or (id=(select id from usrInfo where name=\'%3\') and friendId = (select id from usrInfo where name=\'%4\'))").arg(pername).arg(name).arg(name).arg(pername);
    qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        return 0;  //双方已经是好友
    }
    else
    {
        data = QString("select online from usrInfo where name=\'%1\'").arg(pername);
        QSqlQuery query;
        query.exec(data);
        if (query.next())
        {
            int ret = query.value(0).toInt();
            if (1 == ret)
            {
                return 1;   //在线
            }
            else if (0 == ret)
            {
                return 2;  //不在线
            }
        }
        else
        {
            return 3;   //不存在
        }
    }

}

//数据库操作：同意添加好友
void OpeDB::handleAgreeAddFriend(const char *pername, const char *name)
{
    if (NULL == pername || NULL == name)
    {
        return;
    }
    QString data = QString("insert into friend(id, friendId) values((select id from usrInfo where name=\'%1\'), (select id from usrInfo where name=\'%2\'))").arg(pername).arg(name);
    QSqlQuery query;
    query.exec(data);
}

//数据库操作：刷新好友列表
QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    if (NULL == name)
    {
        return strFriendList;
    }
    QString data = QString("select name from usrInfo where online=1 and id in (select id from friend where friendId=(select id from usrInfo where name=\'%1\'))").arg(name);
    QSqlQuery query;                                                    //用=只能显示一个好友
    query.exec(data);
    while (query.next())
    {
        strFriendList.append(query.value(0).toString()); //查询 获取值 转化为字符串 添加到列表
        qDebug() << "flush name:" << query.value(0).toString();
    }

    data = QString("select name from usrInfo where online=1 and id in (select friendId from friend where id=(select id from usrInfo where name=\'%1\'))").arg(name);
    query.exec(data);
    while (query.next())
    {
        strFriendList.append(query.value(0).toString());
        qDebug() << "flush name:" << query.value(0).toString();
    }
    return strFriendList;
}

//数据库操作：删除好友
bool OpeDB::handleDelFriend(const char *name, const char *friendName)
{
    if (NULL == name || NULL == friendName)
    {
        return false;
    }

    QString data = QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId=(select id from usrInfo where name=\'%2\')").arg(name).arg(friendName);
    QSqlQuery query;       //删除1的好友2
    query.exec(data);

    data = QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId=(select id from usrInfo where name=\'%2\')").arg(friendName).arg(name);
    query.exec(data);     //删除“friendName”的好友“name”

    return true;
}
