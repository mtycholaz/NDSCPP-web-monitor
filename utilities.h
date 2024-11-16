#ifndef UTILITIES_H
#define UTILITIES_H

#include <vector>
#include <cstdint>
#include <initializer_list>
#include "pixeltypes.h"

class Utilities
{
public:
    // Converts a uint16_t to a vector of bytes (little-endian)
    static std::vector<uint8_t> WORDToBytes(uint16_t value)
    {
        return {static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF)};
    }

    // Converts a uint32_t to a vector of bytes (little-endian)
    static std::vector<uint8_t> DWORDToBytes(uint32_t value)
    {
        return {
            static_cast<uint8_t>(value & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 24) & 0xFF)};
    }

    // Converts a uint64_t to a vector of bytes (little-endian)
    static std::vector<uint8_t> ULONGToBytes(uint64_t value)
    {
        return {
            static_cast<uint8_t>(value & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 32) & 0xFF),
            static_cast<uint8_t>((value >> 40) & 0xFF),
            static_cast<uint8_t>((value >> 48) & 0xFF),
            static_cast<uint8_t>((value >> 56) & 0xFF)};
    }

    // Combines multiple byte arrays into one
    static std::vector<uint8_t> CombineByteArrays(std::initializer_list<std::vector<uint8_t>> arrays)
    {
        std::vector<uint8_t> combined;
        for (const auto &array : arrays)
        {
            combined.insert(combined.end(), array.begin(), array.end());
        }
        return combined;
    }

    // Gets color bytes at a specific offset, handling reversing and RGB swapping
    static std::vector<uint8_t> GetColorBytesAtOffset(const std::vector<CRGB> &LEDs, uint32_t offset, uint32_t count, bool reversed, bool redGreenSwap)
    {
        std::vector<uint8_t> colorBytes;
        if (offset >= LEDs.size())
        {
            return colorBytes;
        }

        uint32_t end = std::min(offset + count, static_cast<uint32_t>(LEDs.size()));
        if (reversed)
        {
            for (int32_t i = end - 1; i >= static_cast<int32_t>(offset); --i)
            {
                AppendColorBytes(colorBytes, LEDs[i], redGreenSwap);
            }
        }
        else
        {
            for (uint32_t i = offset; i < end; ++i)
            {
                AppendColorBytes(colorBytes, LEDs[i], redGreenSwap);
            }
        }
        return colorBytes;
    }

private:
    // Helper to append color bytes to a vector
    static void AppendColorBytes(std::vector<uint8_t> &bytes, const CRGB &color, bool redGreenSwap)
    {
        if (redGreenSwap)
        {
            bytes.push_back(color.g);
            bytes.push_back(color.r);
        }
        else
        {
            bytes.push_back(color.r);
            bytes.push_back(color.g);
        }
        bytes.push_back(color.b);
    }
};

#endif // UTILITIES_H
