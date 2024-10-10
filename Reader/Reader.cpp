#include "Reader.h"
#include "MessageTran.h"
#include "UploadTab.h"
#include <fstream>

using namespace std;

Reader::Reader(const string& ipAddress, int port) : ipAddress(ipAddress), port(port), sock(0) {}

Reader::~Reader() {
    // 断开连接
    if (sock > 0) {
        close(sock);
        sock = 0;
    }
}

bool Reader::connectToServer() {
    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation error" << endl; // cerr用于输出错误信息
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        return false;
    }

    // 连接到服务器
    if (connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Connection failed" << endl;
        close(sock);
        return false;
    }
    cout << "Successfully connected to server" << endl;
    
    
    return true;
}

// 发数据
bool Reader::sendMessage(const vector<uint8_t>& message) {
    if (send(sock, message.data(), message.size(), 0) == -1) {
        cerr << "Failed to send message" << endl;
        return false;
    }
    return true;
}

void Reader::setBlocking(int sock, bool isBlocking) {
    // true为阻塞，false为非阻塞
    int flags = fcntl(sock, F_GETFL, isBlocking);
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        cerr << "fcntl F_SETFL error" << endl;
    }
}

// 收数据
void Reader::receiveData() {
    vector<uint8_t> buffer(0);
    int nRead = 0;
    setBlocking(sock, false);
    // 用延时和非阻塞读取数据
    cout << "接收数据中...";
    while (true) {
        sleep(1);
        vector<uint8_t> tempBuffer(1024); // 临时缓冲区
        nRead = recv(sock, tempBuffer.data(), tempBuffer.size(), 0);
        if (nRead > 0) {
            buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + nRead);
        } else if (nRead < 0) {
            cerr << endl << "连接已关闭";
            // setBlocking(sock, true);
            break;
        } else {
            cerr << "接收数据失败";
            break;
        }
    }
    cout << endl;
    // 集中处理接收到的数据, 并写入txt文件
    ofstream outFile("Data/InventoryData.txt", ios::out | ios::app | ios::binary);
    if (!outFile) {
        cerr << "无法打开文件" << endl;
        return;
    }

    uint8_t* p = buffer.data();
    size_t currentSize = buffer.size();
    while (currentSize > 0) {
        if (*p == 0xa0 && (p + 1) != nullptr) {
            uint8_t length_p = *(p + 1) + 2;
            while(length_p--) {
                outFile << hex << (int)(*p) << " ";
                p++;
                currentSize--;
            }
        } else {
            outFile << hex << (int)(*p) << " ";
            p++;
            currentSize--;
        }
        outFile << endl;
    }
    outFile.close();
    // 清空缓冲区
    memset(buffer.data(), 0, buffer.size());
}

void Reader::GetFirmwareVersion(uint8_t btReadId) {
    uint8_t btCmd = 0x72;
    vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd);
    sendMessage(message);
}

void Reader::SetWorkAntenna(uint8_t btReadId, uint8_t antenna) {
    uint8_t btCmd = 0x74;
    vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, {antenna});
    sendMessage(message);
    receiveData();
}

void Reader::InventoryReal(uint8_t btReadId, const vector<uint8_t>& antennas, uint8_t btRepeat) {
    uint8_t btCmd = 0x89;
    // 设置天线为工作状态
    for(auto antenna : antennas) {
        SetWorkAntenna(btReadId, antenna);
        vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, {btRepeat});
        sendMessage(message);
        receiveData();
    }
}

void Reader::SetAccessEpcMatch(uint8_t btReadId, vector<uint8_t> btAryEPC) {
    /*输入：读写器id; EPC(不包含CRC+PC)*/
    uint8_t btCmd = 0x85;
    uint8_t btMode = 0x00; // 0x00: EPC使能匹配
    uint8_t btEpcLen = (uint8_t)(btAryEPC.size());
    vector<uint8_t> btAryData(2, 0);
    btAryData[0] = btMode;
    btAryData[1] = btEpcLen;
    btAryData.insert(btAryData.end(), btAryEPC.begin(), btAryEPC.end());
    vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    // receiveData();
}

void Reader::CancelAccessEpcMatch(uint8_t btReadId, vector<uint8_t> btAryEPC) {
    /*输入：读写器id; EPC(不包含CRC+PC)*/
    uint8_t btCmd = 0x85;
    uint8_t btMode = 0x01; // 0x00: EPC清除匹配
    uint8_t btEpcLen = (uint8_t)(btAryEPC.size());
    vector<uint8_t> btAryData(2, 0);
    btAryData[0] = btMode;
    btAryData[1] = btEpcLen;
    btAryData.insert(btAryData.end(), btAryEPC.begin(), btAryEPC.end());
    vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    // receiveData();
}

void Reader::WriteEPC(uint8_t btReadId, vector<uint8_t> btAryEPC, vector<uint8_t> btAryPassWord, uint8_t btWordAdd, uint8_t btWordCnt, vector<uint8_t> btArySetData, const vector<uint8_t>& antennas) {
    /*
    输入: 读写器id; 原EPC(不包含CRC+PC); 访问密码; 起始地址; 数据长度; 写入EPC(不包含CRC+PC)
    */
    // 用EPC号选定标签
    SetAccessEpcMatch(btReadId, btAryEPC);
    
    uint8_t btCmd = 0x82; // 0x82为Write,0x92为BlockWrite
    uint8_t btEpcLen = (uint8_t)(btArySetData.size());
    uint8_t btMemBank = 0x01; // 0x01: 选定为EPC区
    
    vector<uint8_t> btAryData;
    btAryData.insert(btAryData.end(), btAryPassWord.begin(), btAryPassWord.end());
    btAryData.push_back(btMemBank);
    btAryData.push_back(btWordAdd);
    btAryData.push_back(btWordCnt);
    btAryData.insert(btAryData.end(), btArySetData.begin(), btArySetData.end());
    
    vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    receiveData();
    
    // 清除匹配
    CancelAccessEpcMatch(btReadId, btAryEPC);
}

void Reader::WriteTag(uint8_t btReadId, int tagID, int batch, int weight, string dbFilePath, const vector<uint8_t>& antennas) {
    // 取得原EPC号
    vector<uint8_t> origin_EPC  = UploadTab::findEPC(tagID, dbFilePath);
    
    // 设置新EPC号
    vector<int> data = {tagID, batch, weight};
    vector<uint8_t> new_EPC = UploadTab::IntToTwoBytes(data);
    
    // 组合报文
    vector<uint8_t> btAryPassWord = {0x00, 0x00, 0x00, 0x00};
    uint8_t btWordAdd = 0x02; // EPC区起始地址
    uint8_t btWordCnt = origin_EPC.size() / 2; // EPC区长度
    WriteEPC(btReadId, origin_EPC, btAryPassWord, btWordAdd, btWordCnt, new_EPC, antennas);
}