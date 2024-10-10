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

using namespace std;

int main() {
    // 定义Server的IP地址和端口
    const string ipAddress = "192.168.0.178"; // 替换为目标IP地址
    int nPort = 4001; // 替换为目标端口

    Reader reader(ipAddress, nPort);
    if(reader.connectToServer() == false) {
        return -1;
    }

    // 清空历史接收数据
    UploadTab uploadTab;
    uploadTab.ClearHistoryData(false, "Data/InventoryData.db");

    uint8_t btReadId = 0x00; // 读写器ID, 公用地址0xFF，广播形式
    reader.GetFirmwareVersion(btReadId);

    // XXX 自动识别天线 cmd_get_ant_physical_connection_status cmd:0x53 -> 舍弃

    vector<uint8_t> antennas = {0x00,0x02}; // 4个天线:00~03

    // 设定天线、启动盘存
    cout << "首次盘存" << endl;
    uint8_t btRepeat = 0x01; // 0xFF: 持续轮询
    reader.InventoryReal(btReadId, antennas, btRepeat);

    // 盘存数据中的EPC数据存入数据库
    cout << "写入数据库" << endl;
    uploadTab.SaveDataToTable("Data/InventoryData.txt", "Data/EPC.db");

    // 用标签号在数据库中查询EPC号
    int tagID = 2;
    int batch = 3;
    int weight = 500;
    cout << "修改标签" << endl;
    reader.WriteTag(btReadId, tagID, batch, weight, "Data/EPC.db", antennas);

    // 更新数据库
    cout << "第二次盘存" << endl;
    // uploadTab.ClearHistoryData(false, "Data/InventoryData.db");
    reader.InventoryReal(btReadId, antennas, btRepeat);
    uploadTab.SaveDataToTable("Data/InventoryData.txt", "Data/EPC.db");
    return 0;
}