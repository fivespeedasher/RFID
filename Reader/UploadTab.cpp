/**
 * @file UploadTab.cpp
 * @author Jushawn
 * @brief 用于sqlite数据库文件的读写
 * @version 0.1
 * @date 2024-10-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "UploadTab.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <iterator>

using namespace std;
// TODO 数据库create、执行等操作分离成单个函数
UploadTab::UploadTab(string dbFilePath) : dbFilePath(dbFilePath) {initDB();}
UploadTab::~UploadTab() {}

static int callback(void* data, int argc, char **argv, char **azColName){
    string* result = static_cast<string*>(data); // 由于sqlite3_exec函数的callback函数的第一个参数是void*类型，所以需要进行强制类型转换
    *result = *argv;
    return 0;
}

/**
 * @brief 清空txt文件与数据库文件
 * 
 * @param isSQLIncluded true 清空txt文件与数据库; false 仅清空txt文件
 * @param dbFilePath db文件路径
 */
void UploadTab::ClearHistoryData(bool isSQLIncluded) {
    // 将txt文件清空
    ofstream outFile("Data/InventoryData.txt", ios::trunc);
    if (!outFile) {
        cerr << "无法打开文件" << endl;
    }
    // XXX：将数据库清空
    if (isSQLIncluded) {}

}

/**
 * @brief 将string转为vector<uint8_t>, 以空格分隔
 * 
 * @param str 
 * @param delimiter 
 * @return vector<uint8_t> 
 */
vector<uint8_t> string_split(const string& str, char delimiter) {
    vector<uint8_t> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter)) {
        uint8_t value = static_cast<uint8_t>(stoi(token, nullptr, 16)); // 将16进制文件传入，无需转换
        tokens.push_back(value);
    }
    return tokens;
}

/**
 * @brief 将两位十六进制字节转为int
 * 
 * @param data 
 * @return vector<int> 
 */
vector<int> UploadTab::TwoBytesToInt(const vector<uint8_t>& data) {
    vector<int> result;
    for (int i = 0; i < data.size(); i += 2) {
        int value = (data[i] << 8) | data[i + 1];
        result.push_back(value);
    }
    return result;
}

/**
 * @brief 将int数组转为两位十六进制字节
 * 
 * @param data 
 * @return vetor<uint8_t> 
 */
vector<uint8_t> UploadTab::IntToTwoBytes(const vector<int>& data) {
    vector<uint8_t> result;
    for (int i = 0; i < data.size(); i++) {
        result.push_back((data[i] >> 8) & 0xFF);
        result.push_back(data[i] & 0xFF);
    }
    return result;
}

int UploadTab::initDB() {
    this->zErrMsg = 0; // 错误信息

    /* 打开数据库 */
    rc = sqlite3_open(dbFilePath.c_str(), &this->db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }

    /* 创建Table */
    this->sql = "CREATE TABLE IF NOT EXISTS BatchTag("  \
            "TagID      INT     PRIMARY KEY NOT NULL," \
            "Batch      INT     NOT NULL," \
            "Weight     INT," \
            "EPC        TEXT);";

    /* 执行SQL命令 */
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table created successfully\n");
    }
    return 0;
}

/**
 * @brief 将txt文件中的EPC数据存入数据库
 * 
 * @param txtFilePath
 */
void UploadTab::SaveDataToTable(string txtFilePath) {
    /* 打开数据库 */
    this->rc = sqlite3_open(dbFilePath.c_str(), &db);
    if( this->rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }

    /* 打开EPC数据存放的TXT文件 */
    ifstream inFile(txtFilePath);
    if (!inFile) {
        cerr << "无法打开文件" << endl;
    }
    string line;

    /* 逐行处理，以10进制存入EPC数据 */
    while (getline(inFile, line)) {
        // 跳过空行、长度不符合、命令号不对的行
        if (line.empty() || line[3] != 'd' || line.substr(7, 2) != "89") {
            continue;
        }

        cout << line << endl;
        // 将string转为vector, 以空格分隔
        vector<uint8_t> msg = string_split(line, ' ');
        // 匹配EPC号
        if(msg[3] = 0x89 && msg[1] != 0xa) {
            vector<uint8_t> EPC;
            EPC.insert(EPC.begin(), msg.begin() + 7, msg.end() - 2); // 实际存入的是不包括CRC和PC位的EPC
            // 截取EPC号
            size_t start = line.find("18 0 ") + 5;
            string EPC_str = line.substr(start, line.length() - start - 7);

            // 将EPC解码并存入数据库
            vector<int> data_int = TwoBytesToInt(EPC);
            // convert data_int to string
            string data_str = "(";
            for (int i = 0; i < data_int.size(); i++) {
                    data_str += to_string(data_int[i]);
                    if (i != data_int.size() - 1) {
                        data_str += ", ";
                    }
                    else data_str += ", \'" + EPC_str + "\')";
            }
            
            // 插入数据，如存在则替换
            sql = "INSERT OR REPLACE INTO BatchTag (TagID, Batch, Weight, EPC) " \
                "VALUES " + data_str + ";";

            // execute SQL命令
            this->rc = sqlite3_exec(this->db, sql.c_str(), callback, 0, &zErrMsg);
            if( this->rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }else{
                // fprintf(stdout);
                fprintf(stdout, "Data insert successfully\n");
            }
        }

    }
    inFile.close();
}

vector<uint8_t> UploadTab::findEPC(int tagID) {
    // 打开数据库
    this->rc = sqlite3_open(dbFilePath.c_str(), &this->db);
    if( this->rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }

    // 查询EPC
    // 查找TagID的对应的EPC号
    sql = "SELECT EPC FROM BatchTag WHERE TagID = " + to_string(tagID) + ";";
    string EPC_str;
    this->rc = sqlite3_exec(this->db, sql.c_str(), callback, &EPC_str, &zErrMsg);
    if( this->rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Data access successfully\n");
    }
    
    // 将string转为vector<uint8_t>, 以空格分隔
    vector<uint8_t> EPC = string_split(EPC_str, ' ');
    return EPC;
}

// // TODO 提取单个，而不是vector
// // 通过Tag获取表格中的数据
// vector<int> UploadTab::getBatch(int tagID) {
//     // 打开数据库
//     this->rc = sqlite3_open(dbFilePath.c_str(), &this->db);
//     if( this->rc ){
//         fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
//         exit(0);
//     }else{
//         fprintf(stdout, "Opened database successfully\n");
//     }

//     // 查询EPC
//     // 查找TagID的对应的EPC号
//     sql = "SELECT Batch FROM BatchTag WHERE TagID = " + to_string(tagID) + ";";
//     string data_str;
//     this->rc = sqlite3_exec(db, sql.c_str(), callback, &data_str, &zErrMsg);
//     if( this->rc != SQLITE_OK ){
//         fprintf(stderr, "SQL error: %s\n", zErrMsg);
//         sqlite3_free(zErrMsg);
//     }else{
//         fprintf(stdout, "Data access successfully\n");
//     }
    
//     // 将string转为vector<uint8_t>, 以空格分隔
//     vector<uint8_t> data = string_split(data_str, ' ');
//     vector<int> result;
//     for (int i = 0; i < data.size(); i++) {
//         result.push_back(data[i]);
//     }
    
//     return result;
// }