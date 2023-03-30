#ifndef ONLINE_H
#define ONLINE_H

#include <QWidget>
#include "protocol.h"

namespace Ui {
class Online;
}

class Online : public QWidget
{
    Q_OBJECT

public:
    explicit Online(QWidget *parent = 0);
    ~Online();

    void showUsr(PDU *pdu);

private slots:
    void on_addFriend_pb_clicked();

  //  void on_online_lw_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_LoginName_highlighted(const QString &arg1);

private:
    Ui::Online *ui;
};

#endif // ONLINE_H
