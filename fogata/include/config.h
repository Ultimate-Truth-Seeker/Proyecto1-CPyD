#ifndef FOGATA_CONFIG_H
#define FOGATA_CONFIG_H

#include <cstdint>
#include <string>

namespace limits {
constexpr int      kMinWidth      = 640;
constexpr int      kMinHeight     = 480;
constexpr int      kMaxWidth      = 7680;
constexpr int      kMaxHeight     = 4320;
constexpr int      kMinParticles  = 1;
constexpr int      kMaxParticles  = 2000000;
constexpr float    kMinIntensity  = 0.10f;
constexpr float    kMaxIntensity  = 5.00f;
constexpr float    kMaxWind       = 3.00f;
}

struct Config {
    int      width       = 1280;
    int      height      = 720;
    int      nParticles  = 3000;
    int      searchWorkload = 300;
    float    intensity   = 1.0f;
    float    wind        = 0.35f;
    uint32_t seed        = 0;
    bool     vsync       = true;
    bool     showHelp    = false;
};

void configPrintUsage(const char* programName);

bool configParse(int argc, char** argv, Config& out, std::string& error);

#endif
