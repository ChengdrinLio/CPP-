#ifndef OPEWIDGET_H
#define OPEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "friend.h"
#include "book.h"
#include <QStackedWidget> //堆栈窗口：每次显示一个窗口的索引

class OpeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OpeWidget(QWidget *parent = 0);
    static OpeWidget &getInstance(); //是单例
    Friend *getFriend(); //单例模式 获得单例
    Book *getBook(); //单例模式 返回单例

signals:

public slots:

private:
    QListWidget *m_pListW;
    Friend *m_pFriend; //窗口friend
    Book *m_pBook; //窗口book

    QStackedWidget *m_pSW;
};

#endif // OPEWIDGET_H
