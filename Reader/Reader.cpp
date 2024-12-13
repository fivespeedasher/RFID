#include "Reader.h"
#include "MessageTran.h"
#include <fstream>
#include "RFIDconstant.h"

using namespace std;

Reader::Reader(const string& ipAddress, int port) : ipAddress(ipAddress), port(port), sock(-1), isRunning(false), txtFile(txtFilePath) {}

Reader::~Reader() {
    stop();
}
bool Reader::connectToServer() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error: Failed to create socket" << endl;
        return false;
    }

    // 将字符串形式的IP地址和端口号转换为 sockaddr_in 结构体，因为 connect 函数的第二个参数需要 sockaddr_in 结构体
    sockaddr_in serverAddress{};    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        cerr << "Error: Invalid IP address" << endl;
        return false;
    }

    // conncet参数：套接字描述符，指向包含目的端地址的结构体指针，结构体长度
    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Error: Failed to connect to server" << endl;
        return false;
    }
    // 开启接收数据的线程
    isRunning = true;
    receiveThread = thread(&Reader::receiveData, this);

    cout << "Connected to server at " << ipAddress << ":" << port << endl;
    return true;
}

// 发数据
bool Reader::sendMessage(const vector<uint8_t>& message) {
    this_thread::sleep_for(chrono::milliseconds(500)); // 速率控制，将线程挂起0.5s
    if (sock < 0) {
        cerr << "Error: No connection established" << endl;
        return false;
    }

    ssize_t bytesSent = send(sock, message.data(), message.size(), 0);
    if (bytesSent != static_cast<ssize_t>(message.size())) { // 发送的字节数不等于消息长度
        cerr << "Error: Failed to send all data" << endl;
        return false;
    }
    return true;
}

// 停止客户端
void Reader::stop() {
    // 加入资源清理逻辑，确保线程和 socket 都能正常关闭
    if (this->isRunning) {
        isRunning = false;

        if (this->sock >= 0) {
            close(this->sock);
            sock = -1;
        }

        if (this->receiveThread.joinable()) {
            this->receiveThread.join();
        }
    }
}

// 收数据
void Reader::receiveData() {
    // vector<uint8_t> buffer(0);
    // 处理接收到的数据, 并写入txt文件
    ofstream outFile(txtFile, ios::out | ios::app | ios::binary);
    if (!outFile) {
        cerr << "无法打开文件" << endl;
        return;
    }
    try {
        const size_t bufferSize = 2048;          // 定义缓冲区大小
        vector<uint8_t> buffer(bufferSize);   // 接收数据的缓冲区

        while (this->isRunning) {                     // 持续接收数据
            ssize_t bytesRead = recv(sock, buffer.data(), buffer.size(), 0);
            if (bytesRead > 0) {                // 成功接收数据
                lock_guard<mutex> lock(streamMutex);
                // buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + bytesRead);
                // 接收到的数据写入文件
                for (size_t i = 0; i < bytesRead; ++i) {
                    outFile << hex << (int)buffer[i] << " ";
                }

                outFile.flush(); // 确保数据立即写入文件
                cout << "Received: ";
                for (size_t i = 0; i < bytesRead; ++i) {
                    cout << hex << (int)buffer[i] << " ";
                }
                cout << endl;
                // 清空buffer
                buffer.clear();
                buffer.resize(bufferSize);
            } else if (bytesRead == 0) {        // 服务器关闭连接
                cout << "Server disconnected." << endl;
                break;
            } else {                            // 读取数据时发生错误
                cerr << "Error: Failed to read data" << endl;
                break;
            }
        }
    } catch (...) {                             // 捕获并忽略接收线程中的异常
            cerr << "Error: Exception in receive thread." << endl;
    }
    this->isRunning = false;                          // 确保标志状态被更新
    outFile.close();
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
    // receiveData();
}

void Reader::InventoryReal(uint8_t btReadId, const vector<uint8_t>& antennas, uint8_t btRepeat) {
    uint8_t btCmd = 0x89;
    // 设置天线为工作状态
    for(auto antenna : antennas) {
        SetWorkAntenna(btReadId, antenna);
        vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, {btRepeat});
        sendMessage(message);
        // receiveData();
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
    // receiveData();
    
    // 清除匹配
    CancelAccessEpcMatch(btReadId, btAryEPC);
}


/**
 * @brief 将int数组转为两位十六进制字节
 * 
 * @param data 
 * @return vetor<uint8_t> 
 */
vector<uint8_t> Reader::IntToTwoBytes(const vector<int>& data) {
    vector<uint8_t> result;
    for (int i = 0; i < data.size(); i++) {
        result.push_back((data[i] >> 8) & 0xFF);
        result.push_back(data[i] & 0xFF);
    }
    return result;
}

// 将新设置的batch、weight写入标签
int Reader::WriteTag(uint8_t btReadId, int tagID, int batch, int weight, string dbFilePath, const vector<uint8_t>& antennas, const vector<uint8_t>& origin_EPC) {
    // 设置新EPC号
    vector<int> data = {tagID, batch, weight};
    vector<uint8_t> new_EPC = Reader::IntToTwoBytes(data);
    
    // 组合报文
    vector<uint8_t> btAryPassWord = {0x00, 0x00, 0x00, 0x00};
    uint8_t btWordAdd = 0x02; // EPC区起始地址
    uint8_t btWordCnt = origin_EPC.size() / 2; // EPC区长度
    if(btWordCnt != data.size()) {
        cerr << "(WriteTag) Error EPC" <<endl;
        return -1;
    }
    else {
        WriteEPC(btReadId, origin_EPC, btAryPassWord, btWordAdd, btWordCnt, new_EPC, antennas);
        return 0;
    }
}
