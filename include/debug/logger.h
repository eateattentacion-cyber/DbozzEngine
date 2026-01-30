#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>

class DebugLogger {
public:
    static DebugLogger& getInstance() {
        static DebugLogger instance;
        return instance;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file << message << std::endl;
            m_file.flush();
        }
    }

    template<typename T>
    DebugLogger& operator<<(const T& value) {
        std::ostringstream oss;
        oss << value;
        m_buffer += oss.str();
        return *this;
    }

    DebugLogger& operator<<(std::ostream& (*pf)(std::ostream&)) {
        log(m_buffer);
        m_buffer.clear();
        return *this;
    }

private:
    DebugLogger() {
        m_file.open("C:\\Users\\User\\Documents\\DabozzEngine\\debug_log.txt", std::ios::out | std::ios::app);
        if (m_file.is_open()) {
            m_file << "\n=== DabozzEngine Debug Log ===" << std::endl;
        }
    }

    ~DebugLogger() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    std::ofstream m_file;
    std::string m_buffer;
    std::mutex m_mutex;
};

#define DEBUG_LOG DebugLogger::getInstance()