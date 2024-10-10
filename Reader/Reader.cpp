#include "Reader.h"
#include "MessageTran.h"


Reader::Reader(const std::string& ipAddress, int port) : ipAddress(ipAddress), port(port), sock(0) {}

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
        std::cerr << "Socket creation error" << std::endl; // cerr用于输出错误信息
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return false;
    }

    // 连接到服务器
    if (connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(sock);
        return false;
    }
    std::cout << "Successfully connected to server" << std::endl;
    
    
    return true;
}

// 发数据
bool Reader::sendMessage(const std::vector<uint8_t>& message) {
    if (send(sock, message.data(), message.size(), 0) == -1) {
        std::cerr << "Failed to send message" << std::endl;
        return false;
    }
    return true;
}

void Reader::setBlocking(int sock, bool isBlocking) {
    // true为阻塞，false为非阻塞
    int flags = fcntl(sock, F_GETFL, isBlocking);
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "fcntl F_SETFL error" << std::endl;
    }
}

// 收数据
void Reader::receiveData() {
    std::vector<uint8_t> buffer(0);
    int nRead = 0;
    setBlocking(sock, false);
    // 用延时和非阻塞读取数据
    std::cout << "接收到的数据: ";
    while (true) {
        sleep(1);
        std::vector<uint8_t> tempBuffer(1024); // 临时缓冲区
        nRead = recv(sock, tempBuffer.data(), tempBuffer.size(), 0);
        if (nRead > 0) {
            buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + nRead);
        } else if (nRead < 0) {
            std::cerr << std::endl << "连接已关闭";
            // setBlocking(sock, true);
            break;
        } else {
            std::cerr << "接收数据失败";
            break;
        }
    }
    std::cout << std::endl;
    // 集中处理接收到的数据
    uint8_t* p = buffer.data();
    size_t currentSize = buffer.size();
    while (currentSize > 0) {
        if (*p == 0xa0 && (p + 1) != nullptr) {
            uint8_t length_p = *(p + 1) + 2;
            while(length_p--) {
                std::cout << std::hex << (int)(*p) << " ";
                p++;
                currentSize--;
            }
        } else {
            std::cout << std::hex << (int)(*p) << " ";
            p++;
            currentSize--;
        }
        std::cout << std::endl;
    }
    // 清空缓冲区
    memset(buffer.data(), 0, buffer.size());
    // memset(tempBuffer.data(), 0, tempBuffer.size());
}

void Reader::GetFirmwareVersion(uint8_t btReadId) {
    uint8_t btCmd = 0x72;
    std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd);
    sendMessage(message);
}

void Reader::SetWorkAntenna(uint8_t btReadId, uint8_t antenna) {
    uint8_t btCmd = 0x74;
    std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, {antenna});
    sendMessage(message);
    receiveData();
}

void Reader::InventoryReal(uint8_t btReadId, const std::vector<uint8_t>& antennas, uint8_t btRepeat) {
    uint8_t btCmd = 0x89;
    // 设置天线为工作状态
    for(auto antenna : antennas) {
        SetWorkAntenna(btReadId, antenna);
        std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, {btRepeat});
        sendMessage(message);
        receiveData();
    }
}

void Reader::SetAccessEpcMatch(uint8_t btReadId, std::vector<uint8_t> btAryEPC) {
    /*输入：读写器id; EPC(不包含CRC+PC)*/
    uint8_t btCmd = 0x85;
    uint8_t btMode = 0x00; // 0x00: EPC使能匹配
    uint8_t btEpcLen = (uint8_t)(btAryEPC.size());
    std::vector<uint8_t> btAryData(2, 0);
    btAryData[0] = btMode;
    btAryData[1] = btEpcLen;
    btAryData.insert(btAryData.end(), btAryEPC.begin(), btAryEPC.end());
    std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    // receiveData();
}

void Reader::CancelAccessEpcMatch(uint8_t btReadId, std::vector<uint8_t> btAryEPC) {
    /*输入：读写器id; EPC(不包含CRC+PC)*/
    uint8_t btCmd = 0x85;
    uint8_t btMode = 0x01; // 0x00: EPC清除匹配
    uint8_t btEpcLen = (uint8_t)(btAryEPC.size());
    std::vector<uint8_t> btAryData(2, 0);
    btAryData[0] = btMode;
    btAryData[1] = btEpcLen;
    btAryData.insert(btAryData.end(), btAryEPC.begin(), btAryEPC.end());
    std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    // receiveData();
}

void Reader::WriteEPC(uint8_t btReadId, std::vector<uint8_t> btAryEPC, std::vector<uint8_t> btAryPassWord, uint8_t btWordAdd, uint8_t btWordCnt, vector<uint8_t> btArySetData, const vector<uint8_t>& antennas) {
    /*
    输入: 读写器id; 原EPC(不包含CRC+PC); 访问密码; 起始地址; 数据长度; 写入EPC(不包含CRC+PC)
    */
    // 用EPC号选定标签
    SetAccessEpcMatch(btReadId, btAryEPC);
    
    uint8_t btCmd = 0x94; // 0x82也是写入
    uint8_t btEpcLen = (uint8_t)(btArySetData.size());
    uint8_t btMemBank = 0x01; // 0x01: 选定为EPC区
    
    std::vector<uint8_t> btAryData;
    btAryData.insert(btAryData.end(), btAryPassWord.begin(), btAryPassWord.end());
    btAryData.push_back(btMemBank);
    btAryData.push_back(btWordAdd);
    btAryData.push_back(btWordCnt);
    btAryData.insert(btAryData.end(), btArySetData.begin(), btArySetData.end());
    
    std::vector<uint8_t> message = MessageTran::CreateMessage(btReadId, btCmd, btAryData);
    sendMessage(message);
    receiveData();
    
    // 清除匹配
    CancelAccessEpcMatch(btReadId, btAryEPC);
}

void WriteTag(uint8_t btReadId, int tagID, int batch, int weight, string dbFilePath, const vector<uint8_t>& antennas) {
    
}