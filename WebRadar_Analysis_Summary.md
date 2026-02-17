# WebRadar Player Position Mapping - Complete Analysis

## Executive Summary

The WebRadar system in DragonBurn is a complete real-time web-based radar visualization that displays player positions from CS2. It consists of multiple layers:

1. **Backend (C++)**: Collects player data and converts it to JSON
2. **Web Server**: Serves HTML and streams data via Server-Sent Events (SSE)
3. **Frontend (JavaScript)**: Renders players as colored dots on an HTML5 Canvas

The coordinate transformation from world/game space to radar space happens in both C++ (partial transformation with scale/offset) and JavaScript (final pixel mapping with Y-axis flip).

---

## Architecture Overview

```
CS2 Game Engine
    ↓
EntityBatchProcessor (reads player entity data)
    ↓
WebRadarManager::UpdateRadarData() (collects player positions)
    ↓
WebRadarManager::BuildJSON() (serializes to JSON)
    ↓
WebServer::UpdateRadarData() (stores JSON)
    ↓
WebServer::ServerThread() + SSE endpoint
    ↓
Browser EventSource /events
    ↓
JavaScript render() function
    ↓
Canvas with dots for each player
```

---

## Key Components and Files

### 1. BACKEND - Data Collection and Serialization

#### File: `DragonBurn-usermode/Features/WebRadar.h`
**Purpose**: Interface and configuration for WebRadar manager

**Key Structures**:
```cpp
struct PlayerData {
    Vec3 position;           // World X, Y, Z coordinates
    Vec3 cameraPos;          // Camera position (head position)
    int health;              // Player health (0-100)
    int teamId;              // 2 = CT (blue), 3 = T (red)
    std::string weaponName;  // Current weapon
    std::string playerName;  // Player name
    Vec2 viewAngle;          // View angles (pitch, yaw)
    int entityIndex;         // Entity index in game
};

struct MapData {
    std::string name;        // Map name (e.g., "de_dust2")
    float scale;             // MapScale config value
    float offsetX;           // X offset config value
    float offsetY;           // Y offset config value
};
```

**Configuration Namespace**:
```cpp
namespace WebRadarCFG {
    inline bool Enabled = false;      // Toggle enable/disable
    inline int Port = 8080;           // HTTP server port
    inline float MapScale = 4.0f;     // World-to-radar scale factor
    inline float OffsetX = 0.0f;      // X-axis offset
    inline float OffsetY = 0.0f;      // Y-axis offset
}
```

#### File: `DragonBurn-usermode/Features/WebRadar.cpp`
**Purpose**: Core WebRadar logic and JSON serialization

**Key Functions**:

##### `UpdateRadarData()`
- Called once per game tick (every ~16ms)
- Iterates through all entity pairs (index, entity)
- Filters for alive players only
- Collects PlayerData struct for each player
- Gets current map name from game memory
- Calls BuildJSON() to serialize
- Sends JSON to WebServer via UpdateRadarData()

**Code Flow**:
```cpp
void UpdateRadarData(
    const std::vector<std::pair<int, CEntity>>& entities,  // All entities in world
    const CEntity& localEntity,                             // Local player
    int localPlayerControllerIndex,                         // Local player index
    DWORD tickCount)                                        // Current game tick
{
    // 1. Collect player data
    std::vector<PlayerData> players;
    for (const auto& [entityIndex, entity] : entities) {
        if (!entity.IsAlive()) continue;
        
        PlayerData player;
        player.position = entity.Pawn.Pos;              // RAW world X, Y, Z
        player.cameraPos = entity.Pawn.CameraPos;
        player.health = entity.Pawn.Health;
        player.teamId = entity.Pawn.TeamID;              // 2=CT (blue), 3=T (red)
        // ... more fields
        players.push_back(player);
    }
    
    // 2. Get map data
    MapData mapData;
    mapData.name = GetCurrentMapName();
    mapData.scale = WebRadarCFG::MapScale;              // From config
    mapData.offsetX = WebRadarCFG::OffsetX;             // From config
    mapData.offsetY = WebRadarCFG::OffsetY;             // From config
    
    // 3. Serialize and send
    std::string json = BuildJSON(players, mapData, 
                                 localEntity.Controller.TeamID, tickCount);
    m_server->UpdateRadarData(json);  // Send to WebServer
}
```

##### `BuildJSON()`
- Serializes all player and map data to JSON
- Uses std::ostringstream with fixed precision (2 decimals)
- Creates a single JSON object with structure:
  - `map`: Map metadata with name, scale, offset
  - `localTeamId`: Local player's team (2 or 3)
  - `tickCount`: Current game tick
  - `players`: Array of player objects

**JSON Output Format**:
```json
{
  "map": {
    "name": "de_dust2",
    "scale": 4.0,
    "offsetX": 0.0,
    "offsetY": 0.0
  },
  "localTeamId": 2,
  "tickCount": 12345,
  "players": [
    {
      "entityIndex": 1,
      "position": {"x": 1234.50, "y": 5678.90, "z": 12.30},
      "cameraPos": {"x": 1234.50, "y": 5678.90, "z": 76.30},
      "viewAngle": {"x": 0.00, "y": 45.00},
      "health": 100,
      "teamId": 2,
      "weaponName": "ak47",
      "name": "PlayerName"
    },
    ...
  ]
}
```

**Key Observations**:
- Player positions are RAW world coordinates (no transformation in C++)
- Map scale and offset are sent as metadata, not applied to positions
- The actual coordinate transformation happens in JavaScript

---

### 2. WEB SERVER - HTTP and SSE Streaming

#### File: `DragonBurn-usermode/WebServer/WebServer.h`
**Purpose**: Web server interface

**Key Methods**:
```cpp
bool Start(int port = 8080);           // Start HTTP server
void Stop();                            // Stop HTTP server
bool IsRunning() const;                 // Check if running
void UpdateRadarData(const std::string& jsonData);  // Store JSON
std::string GetCurrentData() const;     // Get stored JSON
```

#### File: `DragonBurn-usermode/WebServer/WebServer.cpp`
**Purpose**: HTTP server implementation using cpp-httplib

**Key Features**:

1. **GET / endpoint** - Serves HTML file
   - Reads `Resources/webradar.html`
   - Returns as `text/html`

2. **GET /api/map-backgrounds endpoint** - Serves map background configuration
   - Reads `Config/map_backgrounds.json`
   - Returns as `application/json`

3. **POST /api/map-backgrounds endpoint** - Saves map background configuration
   - Accepts JSON body with map URLs
   - Writes to `Config/map_backgrounds.json`

4. **OPTIONS /api/map-backgrounds endpoint** - CORS preflight
   - Allows CORS for map background requests

5. **GET /events endpoint** - Server-Sent Events (SSE) for real-time updates
   - **Critical function for WebRadar**
   - Sends data every 50ms (~20 FPS in SSE, but game provides up to 60 FPS)
   - Uses `set_chunked_content_provider` for streaming
   - Format: `data: {JSON}\n\n`
   - Maintains connection open until client disconnects

**SSE Implementation**:
```cpp
svr.Get("/events", [this](const httplib::Request&, httplib::Response& res) {
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("Access-Control-Allow-Origin", "*");
    
    res.set_chunked_content_provider(
        "text/event-stream",
        [this](size_t /*offset*/, httplib::DataSink& sink) {
            if (m_shouldStop.load()) return false;
            
            std::string data = GetCurrentData();  // Get latest JSON
            std::string sse = "data: " + data + "\n\n";
            
            if (!sink.write(sse.c_str(), sse.size())) return false;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return true;
        }
    );
});
```

**Server Details**:
- Binds to `127.0.0.1` only (localhost, not network accessible)
- Port range: 8000-9999 (configurable)
- Thread-safe with mutex protection on JSON data
- Runs in separate thread to not block game

---

### 3. FRONTEND - HTML, Canvas, and Coordinate Transformation

#### File: `DragonBurn-usermode/Resources/webradar.html`
**Purpose**: Complete web interface for radar visualization

**Critical Code Section - Coordinate Transformation**:

**F
