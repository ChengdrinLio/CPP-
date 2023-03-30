#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget> //显示文件的 列表
#include <QPushButton> //操作文件的 按钮
#include <QHBoxLayout>
#include <QVBoxLayout> //布局
#include "protocol.h"
#include <QTimer>

class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = 0);
    void updateFileList(const PDU *pdu); //更新文件列表
    void clearEnterDir();
    QString enterDir();
    void setDownloadStatus(bool status);
    bool getDownloadStatus();
    QString getSaveFilePath();
    QString getShareFileName();

    qint64 m_iTotal;    //总的文件大小
    qint64 m_iRecved;   //已收到多少

signals:

public slots:
    void createDir();
    void flushFile();
    void delDir();
    void renameFile();
    void enterDir(const QModelIndex &index);
    void returnPre();
    void delRegFile();
    void uploadFile();

    void uploadFileData();

    void downloadFile();

    void shareFile();
    void moveFile();
    void selectDestDir();


private:
    QListWidget *m_pBookListW; //显示文件列表
    QPushButton *m_pReturnPB;
    QPushButton *m_pCreateDirPB;
    QPushButton *m_pDelDirPB;
    QPushButton *m_pRenamePB;
    QPushButton *m_pFlushFilePB;
    QPushButton *m_pUploadPB;
    QPushButton *m_pDownLoadPB;
    QPushButton *m_pDelFilePB;
    QPushButton *m_pShareFilePB;
    QPushButton *m_pMoveFilePB;
    QPushButton *m_pSelectDirPB;

    QString m_strEnterDir;
    QString m_strUploadFilePath;

    QTimer *m_pTimer;

    QString m_strSaveFilePath;
    bool m_bDownload;

    QString m_strShareFileName;

    QString m_strMoveFileName;
    QString m_strMoveFilePath;
    QString m_strDestDir;

};

#endif // BOOK_H
