#ifndef READER_H
#define READER_H
#include <iostream>
#include <thread>
#include <functional>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>
#include <string>
#include <fcntl.h>
#include <cstring>
#include <mutex>

using namespace std;
class Reader {
public:
    Reader(const string& ipAddress, int port);
    ~Reader();
    bool connectToServer();
    bool sendMessage(const vector<uint8_t>& message);
    void GetFirmwareVersion(uint8_t btReadId);
    // 设置指定天线为工作天线
    void SetWorkAntenna(uint8_t btReadId, uint8_t antennas);
    // 实时盘存
    void InventoryReal(uint8_t btReadId, const vector<uint8_t>& antennas, uint8_t btRepeat);
    void SetAccessEpcMatch(uint8_t btReadId, vector<uint8_t> btAryEPC);
    void CancelAccessEpcMatch(uint8_t btReadId, vector<uint8_t> btAryEPC);
    void WriteEPC(uint8_t btReadId, vector<uint8_t> btAryEPC, vector<uint8_t> btAryPassWord, \
            uint8_t btWordAdd, uint8_t btWordCnt, vector<uint8_t> btArySetData, const vector<uint8_t>& antennas);
    int WriteTag(uint8_t btReadId, int tagID, int batch, int weight, string dbFilePath, const vector<uint8_t>& antennas, const vector<uint8_t>& origin_EPC);
    void stop();
    bool isRunning;
private:
    void receiveData();
    // void receiveDataThread(function<void(const vector<uint8_t>&)> callback);
    string ipAddress;
    int port;
    int sock;
    thread receiveThread;   // 接收数据的线程
    mutex streamMutex; // 互斥锁
    vector<uint8_t> IntToTwoBytes(const vector<int>& data);
    string txtFile;
};
#endif