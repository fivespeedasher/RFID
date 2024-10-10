#ifndef UPLOADTAB_H
#define UPLOADTAB_H
#include <vector>
#include <cstdint>
#include <string>

class UploadTab {
public:
    UploadTab();
    ~UploadTab();
    std::vector<int> TwoBytesToInt(const std::vector<uint8_t>& data);
    std::vector<uint8_t> IntToTwoBytes(const std::vector<int>& data);
    void ClearHistoryData(bool isSQLIncluded, std::string dbFilePath);
    void SaveDataToTable(std::string txtFilePath, std::string dbFilePath);
    std::vector<uint8_t> findEPC(int tagID, std::string dbFilePath);
};
#endif // UPLOADTAB_H