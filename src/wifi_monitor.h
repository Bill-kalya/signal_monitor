#ifdef _WIN32
#include <windows.h>
#include <wlanapi.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#endif

#include "signal_monitor.h"
#include <vector>
#include <string>

class WiFiMonitor : public SignalMonitor {
private:
#ifdef _WIN32
    HANDLE hClient;
    DWORD dwVersion;
#endif

public:
    WiFiMonitor() {
#ifdef _WIN32
        hClient = NULL;
        dwVersion = 0;
#endif
    }
    
    ~WiFiMonitor() {
#ifdef _WIN32
        if (hClient) {
            WlanCloseHandle(hClient, NULL);
        }
#endif
    }

    bool initialize() override {
#ifdef _WIN32
        return WlanOpenHandle(1, NULL, &dwVersion, &hClient) == ERROR_SUCCESS;
#else
        return true; // Linux doesn't need special initialization
#endif
    }

    std::vector<SignalData> scan_signals() override {
        std::vector<SignalData> signals;
        
#ifdef _WIN32
        scan_windows_wifi(signals);
#else
        scan_linux_wifi(signals);
#endif
        
        return signals;
    }

private:
#ifdef _WIN32
    void scan_windows_wifi(std::vector<SignalData>& signals) {
        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        if (WlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS) {
            return;
        }

        for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
            PWLAN_BSS_LIST pBssList = NULL;
            if (WlanGetNetworkBssList(hClient, 
                                    &pIfList->InterfaceInfo[i].InterfaceGuid,
                                    NULL, 
                                    dot11_BSS_type_any, 
                                    FALSE, 
                                    NULL, 
                                    &pBssList) == ERROR_SUCCESS && pBssList) {
                
                for (DWORD j = 0; j < pBssList->dwNumberOfItems; j++) {
                    PWLAN_BSS_ENTRY pBssEntry = &pBssList->wlanBssEntries[j];
                    
                    SignalData data;
                    data.timestamp = get_current_timestamp();
                    data.type = "WiFi";
                    data.signal_strength = static_cast<int>(pBssEntry->lRssi);
                    data.quality = get_signal_quality(data.signal_strength);
                    
                    // Extract SSID
                    if (pBssEntry->dot11Ssid.uSSIDLength > 0) {
                        data.network_name = std::string(
                            reinterpret_cast<char*>(pBssEntry->dot11Ssid.ucSSID),
                            pBssEntry->dot11Ssid.uSSIDLength
                        );
                    } else {
                        data.network_name = "Hidden Network";
                    }
                    
                    // Extract MAC address
                    char mac[18];
                    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                            pBssEntry->dot11Bssid[0], pBssEntry->dot11Bssid[1],
                            pBssEntry->dot11Bssid[2], pBssEntry->dot11Bssid[3],
                            pBssEntry->dot11Bssid[4], pBssEntry->dot11Bssid[5]);
                    data.mac_address = mac;
                    
                    data.frequency = static_cast<int>(pBssEntry->ulChCenterFrequency / 1000); // Convert to MHz
                    
                    signals.push_back(data);
                }
                WlanFreeMemory(pBssList);
            }
        }
        
        if (pIfList) {
            WlanFreeMemory(pIfList);
        }
    }
#else
    void scan_linux_wifi(std::vector<SignalData>& signals) {
        // Linux implementation using iwconfig or /proc/net/wireless
        // This is a simplified version - you'd want to use iwlib or parse iw scan results
        FILE* pipe = popen("iw dev 2>/dev/null | grep ssid", "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                // Parse iw output (simplified)
                SignalData data;
                data.timestamp = get_current_timestamp();
                data.type = "WiFi";
                data.signal_strength = -65; // Placeholder
                data.quality = get_signal_quality(data.signal_strength);
                data.network_name = "Linux WiFi Network";
                signals.push_back(data);
            }
            pclose(pipe);
        }
    }
#endif
};