#include "WebServer.h"
#include "../Libs/httplib/httplib.h"
#include "../Helpers/Logger.h"
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

namespace WebRadar
{
    WebServer::WebServer()
        : m_running(false)
        , m_shouldStop(false)
        , m_port(8080)
        , m_currentData("{}")
    {
    }

    WebServer::~WebServer()
    {
        Stop();
    }

    bool WebServer::Start(int port)
    {
        if (m_running.load())
        {
            Log::Warning("WebRadar server is already running");
            return false;
        }

        m_port = port;
        m_shouldStop.store(false);

        try
        {
            m_serverThread = std::make_unique<std::thread>(&WebServer::ServerThread, this);
            
            // Wait a bit for server to start
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (m_running.load())
            {
                Log::Fine("WebRadar server started on http://localhost:" + std::to_string(m_port));
                return true;
            }
            else
            {
                Log::Error("Failed to start WebRadar server");
                return false;
            }
        }
        catch (const std::exception& e)
        {
            Log::Error("WebRadar server exception: " + std::string(e.what()));
            return false;
        }
    }

    void WebServer::Stop()
    {
        if (!m_running.load())
            return;

        m_shouldStop.store(true);
        
        if (m_serverThread && m_serverThread->joinable())
        {
            m_serverThread->join();
        }

        m_running.store(false);
        Log::Info("WebRadar server stopped");
    }

    bool WebServer::IsRunning() const
    {
        return m_running.load();
    }

    void WebServer::UpdateRadarData(const std::string& jsonData)
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_currentData = jsonData;
    }

    std::string WebServer::GetCurrentData() const
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        return m_currentData;
    }

    std::string WebServer::ReadMapBackgroundsConfig()
    {
        std::ifstream file("Config/map_backgrounds.json");
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
        return "{}";
    }

    bool WebServer::WriteMapBackgroundsConfig(const std::string& jsonContent)
    {
        std::ofstream file("Config/map_backgrounds.json");
        if (file.is_open()) {
            file << jsonContent;
            file.close();
            return true;
        }
        return false;
    }

    void WebServer::ServerThread()
    {
        httplib::Server svr;

        // Serve the HTML interface from file
        svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
            std::ifstream file("Resources/webradar.html");
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_content(buffer.str(), "text/html");
            } else {
                res.set_content("<html><body><h1>Error: webradar.html not found</h1></body></html>", "text/html");
            }
        });

        // Serve map coordinate configs
        svr.Get("/map_configs.json", [](const httplib::Request&, httplib::Response& res) {
            std::ifstream file("Resources/map_configs.json");
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_header("Content-Type", "application/json");
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(buffer.str(), "application/json");
            } else {
                res.status = 404;
                res.set_content("{\"error\": \"map_configs.json not found\"}", "application/json");
            }
        });

        // GET map backgrounds configuration
        svr.Get("/api/map-backgrounds", [this](const httplib::Request&, httplib::Response& res) {
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            std::string config = ReadMapBackgroundsConfig();
            res.set_content(config, "application/json");
        });

        // POST to update map backgrounds configuration
        svr.Post("/api/map-backgrounds", [this](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            
            if (WriteMapBackgroundsConfig(req.body)) {
                res.set_content("{\"status\": \"success\", \"message\": \"Map backgrounds updated\"}", "application/json");
            } else {
                res.status = 500;
                res.set_content("{\"status\": \"error\", \"message\": \"Failed to write config\"}", "application/json");
            }
        });

        // CORS preflight
        svr.Options("/api/map-backgrounds", [](const httplib::Request&, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");
            res.set_content("", "text/plain");
        });

        // Download map image endpoint
        svr.Post("/api/download-map-image", [](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            
            try {
                // Parse request JSON
                std::string mapName, imageUrl;
                size_t mapPos = req.body.find("\"mapName\":\"");
                size_t urlPos = req.body.find("\"imageUrl\":\"");
                
                if (mapPos != std::string::npos && urlPos != std::string::npos) {
                    mapPos += 11;
                    size_t mapEnd = req.body.find("\"", mapPos);
                    mapName = req.body.substr(mapPos, mapEnd - mapPos);
                    
                    urlPos += 12;
                    size_t urlEnd = req.body.find("\"", urlPos);
                    imageUrl = req.body.substr(urlPos, urlEnd - urlPos);
                }
                
                if (mapName.empty() || imageUrl.empty()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Missing mapName or imageUrl\"}", "application/json");
                    return;
                }
                
                // Create maps directory if it doesn't exist
                CreateDirectoryA("Resources", NULL);
                CreateDirectoryA("Resources\\maps", NULL);
                
                // Download using WinINet (supports HTTPS)
                HINTERNET hInternet = InternetOpenA("DragonBurn/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                if (!hInternet) {
                    res.status = 500;
                    res.set_content("{\"error\": \"Failed to initialize WinINet\"}", "application/json");
                    return;
                }
                
                HINTERNET hConnect = InternetOpenUrlA(hInternet, imageUrl.c_str(), NULL, 0, 
                    INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
                
                if (!hConnect) {
                    InternetCloseHandle(hInternet);
                    res.status = 500;
                    res.set_content("{\"error\": \"Failed to connect to URL\"}", "application/json");
                    return;
                }
                
                // Read the image data
                std::string imageData;
                char buffer[4096];
                DWORD bytesRead = 0;
                
                while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                    imageData.append(buffer, bytesRead);
                }
                
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hInternet);
                
                if (!imageData.empty()) {
                    // Save to file
                    std::string filename = "Resources/maps/" + mapName + ".png";
                    std::ofstream outFile(filename, std::ios::binary);
                    if (outFile.is_open()) {
                        outFile.write(imageData.c_str(), imageData.size());
                        outFile.close();
                        
                        std::string localPath = "/maps/" + mapName + ".png";
                        res.set_content("{\"success\": true, \"localPath\": \"" + localPath + "\"}", "application/json");
                        Log::Info("Downloaded map image: " + mapName);
                    } else {
                        res.status = 500;
                        res.set_content("{\"error\": \"Failed to save file\"}", "application/json");
                    }
                } else {
                    res.status = 500;
                    res.set_content("{\"error\": \"Failed to download image data\"}", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content("{\"error\": \"" + std::string(e.what()) + "\"}", "application/json");
            }
        });

        // Serve local map images
        svr.Get(R"(/maps/(.+))", [](const httplib::Request& req, httplib::Response& res) {
            std::string filename = "Resources/maps/" + req.matches[1].str();
            std::ifstream file(filename, std::ios::binary);
            if (file.is_open()) {
                std::ostringstream ss;
                ss << file.rdbuf();
                res.set_header("Content-Type", "image/png");
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Cache-Control", "public, max-age=86400");
                res.set_content(ss.str(), "image/png");
            } else {
                res.status = 404;
                res.set_content("Image not found", "text/plain");
            }
        });

        // SSE endpoint for real-time updates
        svr.Get("/events", [this](const httplib::Request&, httplib::Response& res) {
            res.set_header("Content-Type", "text/event-stream");
            res.set_header("Cache-Control", "no-cache");
            res.set_header("Connection", "keep-alive");
            res.set_header("Access-Control-Allow-Origin", "*");

            res.set_chunked_content_provider(
                "text/event-stream",
                [this](size_t /*offset*/, httplib::DataSink& sink) {
                    if (m_shouldStop.load()) {
                        return false;
                    }

                    std::string data = GetCurrentData();
                    std::string sse = "data: " + data + "\n\n";
                    
                    if (!sink.write(sse.c_str(), sse.size())) {
                        return false;
                    }

                    // Update rate: 60 FPS for smoother movement
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    return true;
                }
            );
        });

        // Set server to listen
        svr.set_read_timeout(5, 0);
        svr.set_write_timeout(5, 0);

        m_running.store(true);
        
        // Listen on all network interfaces (0.0.0.0)
        if (!svr.listen("0.0.0.0", m_port)) {
            m_running.store(false);
            Log::Error("Failed to bind WebRadar server to port " + std::to_string(m_port));
        }

        m_running.store(false);
    }
}
