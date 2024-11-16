#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <chrono>
#include "global.h"
#include "utilities.h" // Include the Utilities header

class NightDriverClient
{
public:
    uint32_t FramesPerBuffer = 21;              // How many buffer frames the chips have
    double PercentBufferUse = 0.85;             // How much of the buffer we should use
    bool Reversed = false;                      // Whether the strip layout is reversed

    NightDriverClient(const std::string &hostName,
                      const std::string &friendlyName,
                      bool compressData,
                      uint32_t width,
                      uint32_t height = 1,
                      uint32_t offset = 0,
                      bool reversed = false,
                      uint8_t channel = 0,
                      bool redGreenSwap = false,
                      uint32_t batchSize = 1)
        : HostName(hostName),
          FriendlyName(friendlyName),
          CompressData(compressData),
          Width(width),
          Height(height),
          Offset(offset),
          Reversed(reversed),
          Channel(channel),
          RedGreenSwap(redGreenSwap),
          BatchSize(batchSize)
    {
    }

    double GetTimeOffset(uint32_t framesPerSecond) const
    {
        if (framesPerSecond == 0)
        {
            return 0.0; // Assume 1 second as default if FPS is not available
        }

        return FramesPerBuffer * PercentBufferUse / framesPerSecond;
    }

    std::vector<uint8_t> GetPixelData(const std::vector<CRGB> &LEDs) const
    {
        return Utilities::GetColorBytesAtOffset(LEDs, Offset, Width * Height, Reversed, RedGreenSwap);
    }

    std::vector<uint8_t> GetDataFrame(const std::vector<CRGB> &MainLEDs, const std::chrono::time_point<std::chrono::system_clock> &timeStart, uint32_t framesPerSecond)
    {
        double timeOffset = GetTimeOffset(framesPerSecond);

        // Calculate epoch time in seconds and microseconds
        auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(timeStart.time_since_epoch()).count();
        double adjustedEpoch = epoch / 1e6 + timeOffset;
        uint64_t seconds = static_cast<uint64_t>(adjustedEpoch);
        uint64_t microseconds = static_cast<uint64_t>((adjustedEpoch - seconds) * 1e6);

        auto pixelData = GetPixelData(MainLEDs);

        // Wrap arguments in braces to pass as an initializer list
        return Utilities::CombineByteArrays({
            Utilities::WORDToBytes(WIFI_COMMAND_PIXELDATA64),
            Utilities::WORDToBytes(Channel),
            Utilities::DWORDToBytes(static_cast<uint32_t>(pixelData.size() / 3)),
            Utilities::ULONGToBytes(seconds),
            Utilities::ULONGToBytes(microseconds),
            pixelData});
    }

private:
    std::string HostName;
    std::string FriendlyName;
    bool CompressData;
    uint32_t Width;
    uint32_t Height;
    uint32_t Offset;
    uint8_t Channel;
    bool RedGreenSwap;
    uint32_t BatchSize;

    // Wi-Fi Commands
    static constexpr uint16_t WIFI_COMMAND_PIXELDATA64 = 3;
};
