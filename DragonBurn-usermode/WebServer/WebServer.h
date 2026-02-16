#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

namespace WebRadar
{
    class WebServer
    {
    public:
        WebServer();
        ~WebServer();

        bool Start(int port = 8080);
        void Stop();
        bool IsRunning() const;
        
        void UpdateRadarData(const std::string& jsonData);
        std::string GetCurrentData() const;

    private:
        void ServerThread();
        
        std::atomic<bool> m_running;
        std::atomic<bool> m_shouldStop;
        std::unique_ptr<std::thread> m_serverThread;
        
        mutable std::mutex m_dataMutex;
        std::string m_currentData;
        int m_port;
    };
}
