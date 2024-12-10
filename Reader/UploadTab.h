#ifndef UPLOADTAB_H
#define UPLOADTAB_H
#include <vector>
#include <cstdint>
#include <string>
#include <sqlite3.h> 
using namespace std;
class UploadTab {
public:
    UploadTab(string dbFilePath);
    ~UploadTab();
    int initDB();
    vector<int> TwoBytesToInt(const vector<uint8_t>& data);
    static vector<uint8_t> IntToTwoBytes(const vector<int>& data);
    void ClearHistoryData(bool isSQLIncluded);
    void SaveDataToTable(string txtFilePath);
    vector<uint8_t> findEPC(int tagID);
    vector<int> findData(int tagID);
private:
    string dbFilePath;
    sqlite3 *db;
    char *zErrMsg; // 错误信息
    int  rc;
    string sql;
};
#endif // UPLOADTAB_H