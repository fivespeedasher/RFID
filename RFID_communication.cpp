#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <functional>
#include "Reader/Reader.h"
#include "Reader/MessageTran.h"
#include "Reader/UploadTab.h"
#include "Reader/RFIDconstant.h"

using namespace std;

int main() {
    // 定义Server的IP地址和端口
    const string ipAddress = "192.168.1.7"; // 目标IP地址
    int nPort = 4001; // 目标端口

    Reader reader(ipAddress, nPort);
    if(reader.connectToServer() == false) {
        return -1;
    }

    // UploadTab uploadTab(dbFilePath);
    // // 清空历史接收数据
    // uploadTab.ClearHistoryData(false);

    // 读取固件版本
    reader.GetFirmwareVersion(btReadId);

    // 盘存，写入txt文件
    reader.InventoryReal(btReadId, antennas, btRepeat);

    // // // 盘存数据中的EPC数据存入数据库
    // uploadTab.SaveDataToTable(txtFilePath, dbFilePath);

    // 用标签号在数据库中查询EPC号
    int tagID = 2;
    // vector<uint8_t> origin_EPC  = uploadTab.findEPC(tagID);

    // vector<int> tag_info;
    // tag_info = uploadTab.findData(tagID);
    // cout << tag_info[1] << endl;
    // 更新批次、重量信息
    int batch;
    int weight;
    batch = 3;
    weight = 11;

    // // 修改EPC信息
    // cout << "修改标签" << endl;
    // reader.WriteTag(btReadId, tagID, batch, weight, dbFilePath, antennas, origin_EPC);

    // // 更新数据库
    reader.InventoryReal(btReadId, antennas, btRepeat);
    // uploadTab.SaveDataToTable(txtFilePath);

    return 0;
}