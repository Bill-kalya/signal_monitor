#include "signal_monitor.h"
#include "wifi_monitor.h"
#include "cellular_monitor.h"
#include "data_logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <memory>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signal_handler(int signal) {
    std::cout << "\nShutting down signal monitor...\n";
    running = false;
}

class SuperSignalMonitor {
private:
    std::unique_ptr<WiFiMonitor> wifi_monitor;
    std::unique_ptr<CellularMonitor> cellular_monitor;
    std::unique_ptr<DataLogger> data_logger;
    bool wifi_available;
    bool cellular_available;
    int scan_count;

public:
    SuperSignalMonitor() : wifi_available(false), cellular_available(false), scan_count(0) {
        // Setup signal handler for graceful shutdown
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        // Initialize data logger
        data_logger = std::make_unique<DataLogger>();
        
        // Initialize WiFi monitoring
        wifi_monitor = std::make_unique<WiFiMonitor>();
        wifi_available = wifi_monitor->initialize();
        
        // Initialize cellular monitoring
        cellular_monitor = std::make_unique<CellularMonitor>();
        cellular_available = cellular_monitor->initialize();
        
        std::cout << "=== SUPER SIGNAL MONITOR INITIALIZED ===\n";
        std::cout << "WiFi Monitoring: " << (wifi_available ? "Available" : "Unavailable") << "\n";
        std::cout << "Cellular Monitoring: " << (cellular_available ? "Available" : "Unavailable") << "\n";
        std::cout << "Data Logging: Enabled (CSV + SQLite)\n";
        std::cout << "Press Ctrl+C to stop monitoring and generate report\n";
        std::cout << "===========================================\n\n";
    }

    void continuous_monitoring() {
        std::cout << "Starting continuous monitoring...\n";
        std::cout << "Scanning every 5 seconds\n\n";

        while (running) {
            auto all_signals = scan_all_signals();
            display_signals(all_signals);
            
            // Log data
            data_logger->log_signals(all_signals);
            
            scan_count++;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        // Generate final report
        generate_final_report();
    }

    std::vector<SignalData> scan_all_signals() {
        std::vector<SignalData> all_signals;
        
        if (wifi_available) {
            auto wifi_signals = wifi_monitor->scan_signals();
            all_signals.insert(all_signals.end(), wifi_signals.begin(), wifi_signals.end());
        }
        
        if (cellular_available) {
            auto cellular_signals = cellular_monitor->scan_signals();
            all_signals.insert(all_signals.end(), cellular_signals.begin(), cellular_signals.end());
        }
        
        return all_signals;
    }

    void display_signals(const std::vector<SignalData>& signals) {
        // Clear screen (Unix-like systems)
        std::cout << "\033[2J\033[1;1H";
        
        std::cout << "=== SUPER SIGNAL STRENGTH MONITOR ===\n";
        std::cout << "Scan #: " << scan_count << " | Time: " << SignalMonitor::get_current_timestamp() << "\n";
        std::cout << "Networks found: " << signals.size() << "\n\n";
        
        std::cout << std::left 
                  << std::setw(8) << "Type" 
                  << std::setw(20) << "Network" 
                  << std::setw(8) << "dBm" 
                  << std::setw(12) << "Quality"
                  << std::setw(12) << "Tech" 
                  << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        for (const auto& signal : signals) {
            std::cout << std::left 
                      << std::setw(8) << signal.type
                      << std::setw(20) << (signal.network_name.length() > 19 ? 
                                         signal.network_name.substr(0, 17) + ".." : 
                                         signal.network_name)
                      << std::setw(8) << (signal.signal_strength == 9999 ? "N/A" : 
                                         std::to_string(signal.signal_strength))
                      << std::setw(12) << signal.quality
                      << std::setw(12) << signal.technology
                      << "\n";
        }
        
        std::cout << "\nLegend: Excellent(-50) Good(-60) Fair(-70) Poor(-80) Very Poor(< -80)\n";
        std::cout << "Data is being logged to CSV and SQLite database\n";
    }
    
    void generate_final_report() {
        std::cout << "\n\n=== FINAL MONITORING REPORT ===\n";
        std::cout << "Total scans performed: " << scan_count << "\n";
        std::cout << "Monitoring duration: " << (scan_count * 5) << " seconds\n";
        
        // Generate reports
        data_logger->generate_report();
        data_logger->export_to_kml("signal_data.kml");
        
        std::cout << "\nFiles created:\n";
        std::cout << "  - CSV data file (in data/ folder)\n";
        std::cout << "  - SQLite database (data/signal_data.db)\n";
        std::cout << "  - KML file for Google Earth (data/signal_data.kml)\n";
        std::cout << "\nThank you for using Super Signal Monitor!\n";
    }
};

int main() {
    try {
        SuperSignalMonitor monitor;
        monitor.continuous_monitoring();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}