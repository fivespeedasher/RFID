#include <string>
#include <vector>
using namespace std;

const string dbFilePath = "Data/InventoryData.db";  // EPC存入的数据库
const string txtFilePath = "Data/InventoryData.txt";    // 数据存入的txt
const uint8_t btReadId = 0x00; // 读写器ID, 公用地址0xFF，广播形式
const vector<uint8_t> antennas = {0x01, 0x02, 0x03}; // 设置天线
const uint8_t btRepeat = 0x01; // 单次盘存的轮询次数

