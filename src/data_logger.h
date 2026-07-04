#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "signal_monitor.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <sqlite3.h>
#include <iomanip>

class DataLogger {
private:
    std::string csv_filename;
    std::string sqlite_filename;
    bool sqlite_available;
    
public:
    DataLogger() : sqlite_available(false) {
        // Create data directory if it doesn't exist
        std::filesystem::create_directory("data");
        
        // CSV file with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "data/signal_log_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".csv";
        csv_filename = ss.str();
        
        // SQLite database
        sqlite_filename = "data/signal_data.db";
        initialize_sqlite();
        
        // Initialize CSV file
        initialize_csv();
        
        std::cout << "Data Logger initialized:\n";
        std::cout << "  CSV: " << csv_filename << "\n";
        std::cout << "  SQLite: " << sqlite_filename << "\n";
    }
    
    ~DataLogger() {
        if (csv_file.is_open()) {
            csv_file.close();
        }
    }
    
    void log_signals(const std::vector<SignalData>& signals) {
        log_to_csv(signals);
        log_to_sqlite(signals);
    }
    
    void export_to_kml(const std::string& filename) {
        // Simple KML export for Google Earth
        std::ofstream kml_file("data/" + filename);
        kml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        kml_file << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
        kml_file << "<Document>\n";
        kml_file << "<name>Signal Strength Data</name>\n";
        kml_file << "<description>Mobile and WiFi signal measurements</description>\n";
        
        // Add placemarks for each signal with location
        for (const auto& signal : signals) {
            if (signal.latitude != 0.0 && signal.longitude != 0.0) {
                kml_file << "<Placemark>\n";
                kml_file << "<name>" << signal.type << " - " << signal.signal_strength << " dBm</name>\n";
                kml_file << "<description>\n";
                kml_file << "Network: " << signal.network_name << "\n";
                kml_file << "Strength: " << signal.signal_strength << " dBm\n";
                kml_file << "Quality: " << signal.quality << "\n";
                kml_file << "Technology: " << signal.technology << "\n";
                kml_file << "Time: " << signal.timestamp << "\n";
                kml_file << "</description>\n";
                kml_file << "<Point>\n";
                kml_file << "<coordinates>" << signal.longitude << "," << signal.latitude << ",0</coordinates>\n";
                kml_file << "</Point>\n";
                kml_file << "</Placemark>\n";
            }
        }
        
        kml_file << "</Document>\n";
        kml_file << "</kml>\n";
        kml_file.close();
        
        std::cout << "KML file exported: data/" << filename << "\n";
    }
    
    void generate_report() {
        std::cout << "\n=== SIGNAL DATA REPORT ===\n";
        
        std::ifstream file(csv_filename);
        std::string line;
        int total_entries = 0;
        std::map<std::string, int> signal_counts;
        std::map<std::string, int> technology_counts;
        
        while (std::getline(file, line)) {
            if (total_entries > 0) {
                std::vector<std::string> fields;
                bool in_quotes = false;
                std::string field;
                
                for (char c : line) {
                    if (c == '"') {
                        in_quotes = !in_quotes;
                    } else if (c == ',' && !in_quotes) {
                        fields.push_back(field);
                        field.clear();
                    } else {
                        field += c;
                    }
                }
                fields.push_back(field);
                
                if (fields.size() >= 5) {
                    signal_counts[fields[4]]++;
                }
                if (fields.size() >= 8) {
                    technology_counts[fields[7]]++;
                }
            }
            total_entries++;
        }
        
        std::cout << "Total measurements: " << total_entries - 1 << "\n";
        std::cout << "Signal Quality Distribution:\n";
        for (const auto& [quality, count] : signal_counts) {
            std::cout << "  " << quality << ": " << count << " measurements\n";
        }
        
        if (!technology_counts.empty()) {
            std::cout << "Technologies Found:\n";
            for (const auto& [tech, count] : technology_counts) {
                std::cout << "  " << tech << ": " << count << " measurements\n";
            }
        }
    }

private:
    std::ofstream csv_file;
    
    void initialize_csv() {
        csv_file.open(csv_filename);
        if (csv_file.is_open()) {
            // Write CSV header
            csv_file << "Timestamp,Type,Network,Strength(dBm),Quality,Latitude,Longitude,Technology,CellID,Operator,MAC,Frequency\n";
        }
    }
    
    void log_to_csv(const std::vector<SignalData>& signals) {
        if (!csv_file.is_open()) {
            initialize_csv();
        }
        
        for (const auto& signal : signals) {
            csv_file << signal.timestamp << ","
                     << signal.type << ","
                     << "\"" << signal.network_name << "\","
                     << signal.signal_strength << ","
                     << signal.quality << ","
                     << signal.latitude << ","
                     << signal.longitude << ","
                     << signal.technology << ","
                     << signal.cell_id << ","
                     << signal.operator_name << ","
                     << signal.mac_address << ","
                     << signal.frequency << "\n";
        }
        csv_file.flush();
    }
    
    void initialize_sqlite() {
        sqlite3* db;
        char* error_message = 0;
        
        int rc = sqlite3_open(sqlite_filename.c_str(), &db);
        if (rc) {
            std::cerr << "Can't open SQLite database: " << sqlite3_errmsg(db) << "\n";
            sqlite_available = false;
            return;
        }
        
        // Create table
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS signal_measurements ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "timestamp TEXT NOT NULL,"
            "type TEXT NOT NULL,"
            "network_name TEXT,"
            "signal_strength INTEGER,"
            "quality TEXT,"
            "latitude REAL,"
            "longitude REAL,"
            "technology TEXT,"
            "cell_id INTEGER,"
            "operator_name TEXT,"
            "mac_address TEXT,"
            "frequency INTEGER"
            ");";
        
        rc = sqlite3_exec(db, sql, 0, 0, &error_message);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << error_message << "\n";
            sqlite3_free(error_message);
            sqlite_available = false;
        } else {
            sqlite_available = true;
            std::cout << "SQLite database initialized successfully\n";
        }
        
        sqlite3_close(db);
    }
    
    void log_to_sqlite(const std::vector<SignalData>& signals) {
        if (!sqlite_available) return;
        
        sqlite3* db;
        int rc = sqlite3_open(sqlite_filename.c_str(), &db);
        if (rc) {
            return;
        }
        
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO signal_measurements "
                         "(timestamp, type, network_name, signal_strength, quality, "
                         "latitude, longitude, technology, cell_id, operator_name, "
                         "mac_address, frequency) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            sqlite3_close(db);
            return;
        }
        
        for (const auto& signal : signals) {
            sqlite3_bind_text(stmt, 1, signal.timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, signal.type.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, signal.network_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, signal.signal_strength);
            sqlite3_bind_text(stmt, 5, signal.quality.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 6, signal.latitude);
            sqlite3_bind_double(stmt, 7, signal.longitude);
            sqlite3_bind_text(stmt, 8, signal.technology.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 9, signal.cell_id);
            sqlite3_bind_text(stmt, 10, signal.operator_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 11, signal.mac_address.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 12, signal.frequency);
            
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
};

#endif