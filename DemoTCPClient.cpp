#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <vector>
#include <fstream>


using namespace std;
// 本地客户端类
class LocalClient {
public:
    LocalClient() : clientSocket(-1), isRunning(false) {}

    ~LocalClient() {
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
        receiveThread = thread(&LocalClient::receiveData, this);

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
        ofstream outFile("Data/InventoryData_Demo.txt", ios::out | ios::app | ios::binary);
        if (!outFile) {
            cerr << "无法打开文件" << endl;
            return;
        }
        try {
            const size_t bufferSize = 1024;          // 定义缓冲区大小
            vector<char> buffer(bufferSize);   // 接收数据的缓冲区

            while (isRunning) {                     // 持续接收数据
                ssize_t bytesRead = recv(clientSocket, buffer.data(), bufferSize, 0);
                if (bytesRead > 0) {                // 成功接收数据
                    lock_guard<mutex> lock(streamMutex);
                    string receivedMessage(buffer.data(), bytesRead);
                    cout << "Received: " << receivedMessage << endl;
                    // 接收到的数据写入文件
                    outFile << receivedMessage << endl;
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
        // 关闭文件
        outFile.close();
    }
};


int main() {
    LocalClient client;

    string ipAddress = "127.0.0.1";
    int port = 12345;

    if (!client.connectToServer(ipAddress, port)) {
        return 1;
    }

    string message;
    cout << "Enter a message to send (type 'exit' to quit): ";
    while (getline(cin, message)) {
        if (message == "exit") {
            break;
        }

        client.sendData(message);
        cout << "Enter a message to send (type 'exit' to quit): ";
    }
    
    client.stop();
    return 0;
}
