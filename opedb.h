#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase> //连接数据库
#include <QSqlQuery>  //查询数据库
#include <QStringList> //字符串列表 显示好友

class OpeDB : public QObject
{
    Q_OBJECT  //支持信号槽
public:
    explicit OpeDB(QObject *parent = 0);
    static OpeDB& getInstance(); //将数据库操作类定义成单例
    //(把它定义成静态的成员函数，在静态的成员函数里面定义一个静态的对象，
    //每次通过类名调用这个静态成员函数的时候，使用这同一个个局部变量

    void init();
    ~OpeDB();

    bool handleRegist(const char *name, const char *pwd); //数据库注册
    bool handleLogin(const char *name, const char *pwd); //数据库登录
    void handleOffline(const char *name); //数据库注册下线
    QStringList handleAllOnline(); //数据库 全部在线好友
    int handleSearchUsr(const char *name); //数据库搜索用户
    int handleAddFriend(const char *pername, const char *name); //数据库添加好友
    void handleAgreeAddFriend(const char *pername, const char *name); //数据库同意添加好友
    QStringList handleFlushFriend(const char *name); //数据库刷新好友
    bool handleDelFriend(const char *name, const char *friendName); //数据库删除好友

signals:

public slots:
private:
    QSqlDatabase m_db;  //连接数据库
};

#endif // OPEDB_H
