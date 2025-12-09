#ifndef SIGNAL_MONITOR_H
#define SIGNAL_MONITOR_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>

struct SignalData {
    std::string timestamp;
    std::string type; // "WiFi", "LTE", "GSM", "5G"
    std::string network_name;
    int signal_strength; // dBm
    std::string quality;
    double latitude;
    double longitude;
    std::string technology; // "802.11ac", "LTE", "NR", etc.
    
    // Cellular specific
    int cell_id;
    int tac; // Tracking Area Code
    std::string operator_name;
    
    // WiFi specific
    std::string mac_address;
    int frequency; // MHz
    std::string encryption;
};

class SignalMonitor {
public:
    virtual bool initialize() = 0;
    virtual std::vector<SignalData> scan_signals() = 0;
    virtual ~SignalMonitor() = default;
    
    static std::string get_signal_quality(int rssi) {
        if (rssi >= -50) return "Excellent";
        else if (rssi >= -60) return "Good";
        else if (rssi >= -70) return "Fair";
        else if (rssi >= -80) return "Poor";
        else return "Very Poor";
    }
    
    static std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        return std::string(buffer);
    }
};

#endif