#include "protocol.h"

//新建pdu 可扩容
PDU *mkPDU(uint uiMsgLen)
{
    uint uiPDULen = sizeof(PDU)+uiMsgLen; //sizeof(PDU) 只算前4个大小
    PDU *pdu = (PDU*)malloc(uiPDULen);
    if (NULL == pdu)
    {
        exit(EXIT_FAILURE);  //申请空间失败，结束函数
    }
    //新申请的内存 初始化
    memset(pdu, 0, uiPDULen);  //将pdu中当前位置后面的uiPDULen个字节用0替换并返回pdu
    pdu->uiPDULen = uiPDULen;  //两个赋值
    pdu->uiMsgLen = uiMsgLen;
    return pdu;
}
