# DragonBurn Codebase Analysis Report

**Generated:** February 17, 2026  
**Repository:** DragonBurn - C++ WebServer & Configuration System

---

## Executive Summary

DragonBurn is a comprehensive C++ gaming cheat tool with an integrated WebServer for remote radar visualization. The codebase demonstrates:
- **Modern C++ patterns** (C++20 standard)
- **Robust JSON-based configuration** persistence (nlohmann/json)
- **HTTP server implementation** using cpp-httplib
- **Real-time data synchronization** via Server-Sent Events (SSE)
- **Modular architecture** with clear separation of concerns

---

## 1. WebServer Architecture

### 1.1 Core WebServer Implementation

**File Location:** `DragonBurn-usermode/WebServer/`
- `WebServer.h` (35 lines)
- `WebServer.cpp` (150 lines)

### 1.2 WebServer Class Design

```cpp
namespace WebRadar {
    class WebServer {
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
```

### 1.3 Key Architecture Patterns

#### 1.3.1 Thread-Safe Operations
- **Thread Model:** Dedicated server thread managed by `std::unique_ptr<std::thread>`
- **Synchronization:** `std::mutex` with `std::lock_guard` for data access
- **Atomics:** `std::atomic<bool>` for non-blocking flag checks
- **Pattern:** Producer-consumer model via `UpdateRadarData()` → `GetCurrentData()`

#### 1.3.2 Request Handling Flow

```
HTTP Request
    ↓
httplib::Server routes request
    ↓
GET "/" → Serves webradar.html from file
GET "/events" → Server-Sent Events (SSE) endpoint
    ↓
Real-time JSON data streamed to client
    ↓
60 FPS update rate (~50ms intervals)
```

#### 1.3.3 Server-Sent Events (SSE) Implementation

**File:** `WebServer.cpp`, lines 108-133

```cpp
svr.Get("/events", [this](const httplib::Request&, httplib::Response& res) {
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("Access-Control-Allow-Origin", "*");
    
    res.set_chunked_content_provider(
        "text/event-stream",
        [this](size_t offset, httplib::DataSink& sink) {
            if (m_shouldStop.load()) {
                return false;
            }
            
            std::string data = GetCurrentData();
            std::string sse = "data: " + data + "\n\n";
            
            if (!sink.write(sse.c_str(), sse.size())) {
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return true;
        }
    );
});
```

**Characteristics:**
- Persistent connection for real-time updates
- Chunked content provider for streaming
- 50ms delay (~20 updates/sec maximum)
- Graceful shutdown via `m_shouldStop` flag
- Localhost binding (127.0.0.1) for security

#### 1.3.4 Static File Serving

**File:** `WebServer.cpp`, lines 96-105

```cpp
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
```

**File I/O Pattern:**
- Direct file stream reading: `std::ifstream`
- Full content buffering: `std::stringstream`
- Fallback error response
- Relative path: `Resources/webradar.html`

### 1.4 Server Lifecycle

```
Start(port)
  └─→ Create server thread
      └─→ httplib::Server::listen()
          ├─→ Bind to 127.0.0.1:port
          ├─→ Set timeouts (5s read/write)
          └─→ Begin accepting connections
      └─→ Wait 100ms for startup
      └─→ Return m_running status

Stop()
  └─→ Signal m_shouldStop = true
  └─→ Join server thread (blocks until shutdown)
  └─→ Set m_running = false
```

### 1.5 Network Configuration

| Property | Value | Location |
|----------|-------|----------|
| Binding Address | 127.0.0.1 (localhost only) | Line 142 |
| Default Port | 8080 | Header |
| Read Timeout | 5 seconds | Line 136 |
| Write Timeout | 5 seconds | Line 137 |
| Update Interval | 50 ms (~20 FPS) | Line 129 |
| Content Type | text/event-stream | Line 109 |

---

## 2. Configuration System

### 2.1 Configuration Architecture

**Locations:**
- `DragonBurn-usermode/Config/ConfigSaver.cpp` (700+ lines)
- `DragonBurn-usermode/Config/ConfigSaver.h` (70 lines)
- `DragonBurn-usermode/Config/ConfigMenu.cpp` (270 lines)
- `DragonBurn-usermode/Config/ConfigMenu.h` (6 lines)
- `DragonBurn-usermode/Core/Config.h` (196 lines)

### 2.2 Configuration Namespaces

```cpp
namespace MenuConfig {
    inline std::string name = "DragonBurn";
    inline std::string version = "3.7.10.4";
    inline std::string author = "ByteCorum";
    inline std::string path = "";  // Config file path
    inline std::string docPath = "";
    inline int RenderFPS = 1000;
    inline int AimDelay = 1;
    inline int BunnyHopDelay = 25;
    // ... window positions, styles, hotkeys
}

namespace ESPConfig {
    inline int HotKey = VK_F6;
    inline bool ESPenabled = true;
    // ... 50+ configuration variables
}

namespace LegitBotConfig {
    inline bool AimBot = true;
    inline float FovLineSize = 60.f;
    // ... aim assist settings
}

namespace RadarCFG {
    inline bool ShowRadar = false;
    inline float RadarRange = 125.f;
    // ... radar visualization settings
}

namespace MiscCFG {
    inline bool BunnyHop = false;
    inline bool WaterMark = true;
    // ... miscellaneous features
}
```

### 2.3 JSON Serialization Pattern

**Libraries Used:**
- `nlohmann/json` (Modern C++ JSON library)
  - Location: `DragonBurn-usermode/Libs/json/json.hpp` (919 KB single-header)
  - Version: 3.11.3

#### 2.3.1 Save Configuration

**File:** `ConfigSaver.cpp`, lines 18-259

```cpp
void SaveConfig(const std::string& filename, const std::string& author) {
    std::ofstream configFile(MenuConfig::path + '\' + filename);
    if (!configFile.is_open()) {
        return;  // Silent failure
    }
    
    json ConfigData;  // nlohmann::json
    
    // Metadata
    ConfigData["0"]["Name"] = MenuConfig::name;
    ConfigData["0"]["Version"] = MenuConfig::version;
    ConfigData["0"]["Author"] = author;
    
    // Nested structure for features
    ConfigData["ESP"]["Hotkey"] = ESPConfig::HotKey;
    ConfigData["ESP"]["Enable"] = ESPConfig::ESPenabled;
    ConfigData["ESP"]["BoneColor"]["r"] = ESPConfig::BoneColor.Value.x;
    // ... hundreds more assignments
    
    // Write to file with 4-space indentation
    configFile << ConfigData.dump(4);
    configFile.close();
}
```

**JSON Output Example Structure:**
```json
{
    "0": {
        "Name": "DragonBurn",
        "Version": "3.7.10.4",
        "Author": "ByteCorum"
    },
    "ESP": {
        "Hotkey": 245,
        "Enable": true,
        "BoneColor": {
            "r": 0.5,
            "g": 0.5,
            "b": 0.5,
            "a": 1.0
        }
    },
    "Aimbot": {
        "Enable": true,
        "Fov": 10.5,
        "CircleColor": { "r": 0.3, "g": 0.7, "b": 1.0, "a": 1.0 }
    }
}
```

#### 2.3.2 Load Configuration

**File:** `ConfigSaver.cpp`, lines 262-450+

```cpp
void LoadConfig(const std::string& filename) {
    json ConfigData;
    std::ifstream configFile(MenuConfig::path + '\' + filename);
    
    if (configFile) {
        configFile >> ConfigData;  // Direct JSON parsing
    } else {
        return;  // Silently return if file doesn't exist
    }
    
    // Safe data extraction with defaults
    if (ConfigData.contains("ESP")) {
        ESPConf
