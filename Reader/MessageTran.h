#ifndef MESSAGETRAN_H
#define MESSAGETRAN_H
#include <vector>
#include <cstdint>

class MessageTran {
public:
    static uint8_t CheckSum(const uint8_t* data, int start, int length);
    static std::vector<uint8_t> CreateMessage(uint8_t readId, uint8_t cmd);
    static std::vector<uint8_t> CreateMessage(uint8_t readId, uint8_t cmd, const std::vector<uint8_t>& btAryData);
};
#endif