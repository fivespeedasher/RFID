#include "Reader.h"
#include "MessageTran.h"
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

    // 设置非阻塞模式
    setBlocking(sock, false);

    // 设置服务器地址
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        return false;
    }

    // 连接到服务器
    int result = connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress));
    if (result == 0) {
        // 非阻塞模式下，立即返回成功
        cout << "Successfully connected to server" << endl;
        return true;
    } else if (errno != EINPROGRESS) {
        // 如果不是正在连接的错误，表示连接失败
        cerr << "Connection failed immediately: " << strerror(errno) << endl;
        close(sock);
        return false;
    }

    // 使用 select 等待连接完成
    fd_set writefds;
    struct timeval tv;
    tv.tv_sec = 2;  // 超时时间 2 秒
    tv.tv_usec = 0;

    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);

    result = select(sock + 1, NULL, &writefds, NULL, &tv);
    if (result <= 0) {
        cerr << (result == 0 ? "Connection timeout " : "Select error: ") << strerror(errno) << endl;
        close(sock);
        return false;
    }

    // 使用 getsockopt 检查是否发生连接错误
    int sockErr;
    socklen_t len = sizeof(sockErr);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &len) < 0 || sockErr != 0) {
        cerr << "Connection error: " << strerror(sockErr) << endl;
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

class Talker {
public:
    Talker() : clientSocket(-1), isRunning(false) {}

    ~Talker() {
        stop();
    }
    // 连接到本地服务器
    bool connectToServer(const string& ipAddress, int port) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
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
        // ::connect中的::表示全局作用域，表示调用的是全局的connect函数
        if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            cerr << "Error: Failed to connect to server" << endl;
            return false;
        }
        // 开启接收数据的线程
        isRunning = true;
        receiveThread = thread(&Talker::receiveData, this);

        cout << "Connected to server at " << ipAddress << ":" << port << endl;
        return true;
    }

    // 发送数据
    bool sendData(const string& message) {
        if (clientSocket < 0) {
            cerr << "Error: No connection established" << endl;
            return false;
        }

        ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
        if (bytesSent != static_cast<ssize_t>(message.size())) { // 发送的字节数不等于消息长度
            cerr << "Error: Failed to send all data" << endl;
            return false;
        }

        cout << "Sent: " << message << endl;
        return true;
    }

    // 停止客户端
    void stop() {
        // 加入资源清理逻辑，确保线程和 socket 都能正常关闭
        if (isRunning) {
            isRunning = false;

            if (clientSocket >= 0) {
                close(clientSocket);
                clientSocket = -1;
            }

            if (receiveThread.joinable()) {
                receiveThread.join();
            }
        }
    }
private:
    int clientSocket;
    bool isRunning;
    thread receiveThread;   // 接收数据的线程
    mutex streamMutex; // 互斥锁

    // 接收数据线程
    void receiveData() {
        /*
         * 功能：
         *  - 独立线程阻塞式监听服务器发送的数据
         *  - 将接收到的消息打印到控制台
         * 改动原因：
         *  - 通过线程将读取数据的逻辑从主线程分离，提高客户端的扩展性。
         */
        try {
            const size_t bufferSize = 1024;          // 定义缓冲区大小
            vector<char> buffer(bufferSize);   // 接收数据的缓冲区

            while (isRunning) {                     // 持续接收数据
                ssize_t bytesRead = recv(clientSocket, buffer.data(), bufferSize, 0);
                if (bytesRead > 0) {                // 成功接收数据
                    lock_guard<mutex> lock(streamMutex);
                    string receivedMessage(buffer.data(), bytesRead);
                    cout << "Received: " << receivedMessage << endl;
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
        isRunning = false;                          // 确保标志状态被更新
    }
};