#include "opewidget.h"

OpeWidget::OpeWidget(QWidget *parent) : QWidget(parent)
{
    m_pListW = new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("图书");


    m_pFriend = new Friend;
    m_pBook = new Book;

    m_pSW = new QStackedWidget; //建立堆栈窗口
    m_pSW->addWidget(m_pFriend); //加入这俩
    m_pSW->addWidget(m_pBook);

    QHBoxLayout *pMain = new QHBoxLayout; //设置水平布局：堆栈窗口在左，索引在右
    pMain->addWidget(m_pListW); //加入这俩
    pMain->addWidget(m_pSW);

    setLayout(pMain);//水平布局
    //窗口变化
    connect(m_pListW, SIGNAL(currentRowChanged(int)) //“当前行变化”信号
            , m_pSW, SLOT(setCurrentIndex(int))); //“设置当前索引”函数
}

//三个单例模式
OpeWidget &OpeWidget::getInstance()
{
    static OpeWidget instance; //产生一个静态的操作界面的对象，不管调用多少次这个函数，使用的都是同一个instance
    return instance; //始终返回的是同一个对象的引用
}

Friend *OpeWidget::getFriend()
{
    return m_pFriend;
}

Book *OpeWidget::getBook()
{
    return m_pBook;
}
