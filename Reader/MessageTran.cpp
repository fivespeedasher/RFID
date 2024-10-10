#include "MessageTran.h"
#include <iostream>

using namespace std;

// 计算校验
uint8_t MessageTran::CheckSum(const uint8_t* data, int start, int length) {
    uint8_t sum = 0;
    for (int i = start; i < length; ++i) {
        sum += data[i];
    }
    sum = ~sum + 1;
    return static_cast<uint8_t>(sum & 0xFF);
}

// 创建消息
vector<uint8_t> MessageTran::CreateMessage(uint8_t readId, uint8_t cmd) {
    vector<uint8_t> message(5, 0);
    message[0] = 0xA0; // 帧头
    message[1] = 0x03; // 长度
    message[2] = readId; // 读写器ID
    message[3] = cmd; // 命令
    message[4] = CheckSum(message.data(), 0, 4);
    return message;
}


vector<uint8_t> MessageTran::CreateMessage(uint8_t readId, uint8_t cmd, const vector<uint8_t>& btAryData) {
    int nLen = btAryData.size() + 5;
    vector<uint8_t> message(nLen, 0);
    message[0] = 0xA0; // 帧头
    message[1] = (uint8_t)(nLen - 2); // 长度
    message[2] = readId; // 读写器ID
    message[3] = cmd; // 命令
    for (int i = 0; i < btAryData.size(); i++) {
        message[i + 4] = btAryData[i];
    }
    message[nLen - 1] = CheckSum(message.data(), 0, nLen - 1);
    return message;
}

// 测试函数
void testMessageTran() {
    vector<uint8_t> data = {0x02, 0x00, 0x24, 0x00, 0x00, 0x00, 0x07};
    vector<uint8_t> message = MessageTran::CreateMessage(0x00, 0x89, data);

    cout << "Message: ";
    for (auto byte : message) {
        cout << hex << (int)byte << " ";
    }
    cout << endl;
}

// 仅在编译测试时包含测试代码
// g++ -DTEST_MESSAGE_TRAN -o MessageTran MessageTran.cpp
#ifdef TEST_MESSAGE_TRAN
int main() {
    testMessageTran();
    return 0;
}
#endif