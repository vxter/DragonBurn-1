#include "WebServer.h"
#include "../Libs/httplib/httplib.h"
#include "../Helpers/Logger.h"
#include <sstream>

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

    void WebServer::ServerThread()
    {
        httplib::Server svr;

        // Serve the HTML interface
        svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DragonBurn WebRadar</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            background: #0a0e1a;
            color: #fff;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            overflow: hidden;
        }
        #header {
            background: linear-gradient(135deg, #1a1f35 0%, #2d3555 100%);
            padding: 15px 20px;
            border-bottom: 2px solid #3d4975;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        #header h1 {
            font-size: 24px;
            color: #4a9eff;
            text-shadow: 0 0 10px rgba(74, 158, 255, 0.5);
        }
        #status {
            display: flex;
            gap: 20px;
            align-items: center;
        }
        .status-item {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 14px;
        }
        .status-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #ff4444;
            box-shadow: 0 0 10px rgba(255, 68, 68, 0.5);
        }
        .status-dot.connected {
            background: #44ff44;
            box-shadow: 0 0 10px rgba(68, 255, 68, 0.5);
        }
        #container {
            display: flex;
            justify-content: center;
            align-items: center;
            height: calc(100vh - 62px);
            padding: 20px;
        }
        #radar-container {
            position: relative;
            width: 1024px;
            height: 1024px;
            background: #0f1419;
            border: 2px solid #3d4975;
            border-radius: 8px;
            box-shadow: 0 0 30px rgba(0, 0, 0, 0.8);
        }
        #radar {
            width: 100%;
            height: 100%;
            display: block;
            image-rendering: -webkit-optimize-contrast;
            image-rendering: crisp-edges;
        }
        #info-panel {
            position: absolute;
            top: 10px;
            right: 10px;
            background: rgba(10, 14, 26, 0.9);
            padding: 15px;
            border-radius: 6px;
            border: 1px solid #3d4975;
            font-size: 12px;
            line-height: 1.6;
            min-width: 200px;
        }
        #info-panel .info-row {
            display: flex;
            justify-content: space-between;
            padding: 3px 0;
        }
        #info-panel .label {
            color: #888;
        }
        #info-panel .value {
            color: #4a9eff;
            font-weight: bold;
        }
        .loading {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            text-align: center;
            color: #888;
        }
        .loading::after {
            content: '...';
            animation: dots 1.5s steps(4, end) infinite;
        }
        @keyframes dots {
            0%, 20% { content: '.'; }
            40% { content: '..'; }
            60%, 100% { content: '...'; }
        }
    </style>
</head>
<body>
    <div id="header">
        <h1>🐉 DragonBurn WebRadar</h1>
        <div id="status">
            <div class="status-item">
                <div class="status-dot" id="connection-status"></div>
                <span id="connection-text">Connecting...</span>
            </div>
            <div class="status-item">
                <span>FPS: <span id="fps">0</span></span>
            </div>
            <div class="status-item">
                <span>Players: <span id="player-count">0</span></span>
            </div>
        </div>
    </div>
    <div id="container">
        <div id="radar-container">
            <canvas id="radar" width="1024" height="1024"></canvas>
            <div id="info-panel">
                <div class="info-row">
                    <span class="label">Map:</span>
                    <span class="value" id="map-name">Unknown</span>
                </div>
                <div class="info-row">
                    <span class="label">Tick:</span>
                    <span class="value" id="tick-count">0</span>
                </div>
                <div class="info-row">
                    <span class="label">Allies:</span>
                    <span class="value" id="ally-count" style="color: #4a9eff;">0</span>
                </div>
                <div class="info-row">
                    <span class="label">Enemies:</span>
                    <span class="value" id="enemy-count" style="color: #ff4444;">0</span>
                </div>
            </div>
            <div class="loading" id="loading">Loading map data</div>
        </div>
    </div>
    <script>
        const canvas = document.getElementById('radar');
        const ctx = canvas.getContext('2d');
        const loading = document.getElementById('loading');
        
        let mapImage = null;
        let currentMapName = '';
        let lastFrameTime = Date.now();
        let frameCount = 0;
        let fps = 0;
        
        const MAP_IMAGES = {
            'de_dust2': 'https://radar-overviews.csgo.saiko.cz/maps/de_dust2_radar.png',
            'de_inferno': 'https://radar-overviews.csgo.saiko.cz/maps/de_inferno_radar.png',
            'de_mirage': 'https://radar-overviews.csgo.saiko.cz/maps/de_mirage_radar.png',
            'de_cache': 'https://radar-overviews.csgo.saiko.cz/maps/de_cache_radar.png',
            'de_overpass': 'https://radar-overviews.csgo.saiko.cz/maps/de_overpass_radar.png',
            'de_nuke': 'https://radar-overviews.csgo.saiko.cz/maps/de_nuke_radar.png',
            'de_train': 'https://radar-overviews.csgo.saiko.cz/maps/de_train_radar.png',
            'de_vertigo': 'https://radar-overviews.csgo.saiko.cz/maps/de_vertigo_radar.png',
            'de_ancient': 'https://radar-overviews.csgo.saiko.cz/maps/de_ancient_radar.png',
            'de_anubis': 'https://radar-overviews.csgo.saiko.cz/maps/de_anubis_radar.png'
        };
        
        function loadMapImage(mapName) {
            if (mapName === currentMapName && mapImage) return;
            
            currentMapName = mapName;
            const imageUrl = MAP_IMAGES[mapName] || MAP_IMAGES['de_dust2'];
            
            const img = new Image();
            img.crossOrigin = 'anonymous';
            img.onload = () => {
                mapImage = img;
                loading.style.display = 'none';
            };
            img.onerror = () => {
                console.error('Failed to load map image:', imageUrl);
                loading.textContent = 'Failed to load map';
            };
            img.src = imageUrl;
        }
        
        function worldToRadar(worldX, worldY, mapScale = 4.0, offsetX = 0, offsetY = 0) {
            const x = (worldX - offsetX) / mapScale;
            const y = (worldY - offsetY) / mapScale;
            return {
                x: Math.floor(x),
                y: Math.floor(1024 - y)
            };
        }
        
        function drawPlayer(x, y, isAlly, health = 100, name = '') {
            const radius = 8;
            
            // Outer glow
            ctx.shadowBlur = 15;
            ctx.shadowColor = isAlly ? '#4a9eff' : '#ff4444';
            
            // Player dot
            ctx.fillStyle = isAlly ? '#4a9eff' : '#ff4444';
            ctx.beginPath();
            ctx.arc(x, y, radius, 0, Math.PI * 2);
            ctx.fill();
            
            // Border
            ctx.shadowBlur = 0;
            ctx.strokeStyle = '#fff';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.arc(x, y, radius, 0, Math.PI * 2);
            ctx.stroke();
            
            // Health indicator (inner circle)
            if (health < 100) {
                ctx.fillStyle = `rgba(255, 68, 68, ${1 - health / 100})`;
                ctx.beginPath();
                ctx.arc(x, y, radius * 0.6, 0, Math.PI * 2);
                ctx.fill();
            }
        }
        
        function render(data) {
            ctx.clearRect(0, 0, 1024, 1024);
            
            // Draw map background
            if (mapImage) {
                ctx.drawImage(mapImage, 0, 0, 1024, 1024);
            } else {
                ctx.fillStyle = '#1a1f35';
                ctx.fillRect(0, 0, 1024, 1024);
            }
            
            if (!data || !data.players) return;
            
            // Update map if changed
            if (data.map && data.map.name !== currentMapName) {
                loadMapImage(data.map.name);
                document.getElementById('map-name').textContent = data.map.name;
            }
            
            // Count teams
            let allyCount = 0;
            let enemyCount = 0;
            
            // Draw players
            data.players.forEach(player => {
                const pos = worldToRadar(player.position.x, player.position.y, 
                    data.map?.scale || 4.0, 
                    data.map?.offsetX || 0, 
                    data.map?.offsetY || 0);
                
                const isAlly = player.teamId === data.localTeamId;
                drawPlayer(pos.x, pos.y, isAlly, player.health, player.name);
                
                if (isAlly) allyCount++;
                else enemyCount++;
            });
            
            // Update stats
            document.getElementById('player-count').textContent = data.players.length;
            document.getElementById('tick-count').textContent = data.tickCount || 0;
            document.getElementById('ally-count').textContent = allyCount;
            document.getElementById('enemy-count').textContent = enemyCount;
            
            // Calculate FPS
            frameCount++;
            const now = Date.now();
            if (now - lastFrameTime >= 1000) {
                fps = Math.round(frameCount * 1000 / (now - lastFrameTime));
                document.getElementById('fps').textContent = fps;
                frameCount = 0;
                lastFrameTime = now;
            }
        }
        
        // Connect to SSE endpoint
        const eventSource = new EventSource('/events');
        
        eventSource.onopen = () => {
            document.getElementById('connection-status').classList.add('connected');
            document.getElementById('connection-text').textContent = 'Connected';
        };
        
        eventSource.onerror = () => {
            document.getElementById('connection-status').classList.remove('connected');
            document.getElementById('connection-text').textContent = 'Disconnected';
        };
        
        eventSource.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                render(data);
            } catch (e) {
                console.error('Failed to parse data:', e);
            }
        };
        
        // Initial render
        render({});
    </script>
</body>
</html>)HTML", "text/html");
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

                    // Update rate: ~60 FPS for smooth rendering
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    return true;
                }
            );
        });

        // Set server to listen
        svr.set_read_timeout(5, 0);
        svr.set_write_timeout(5, 0);

        m_running.store(true);
        
        // Listen on localhost only for security
        if (!svr.listen("127.0.0.1", m_port)) {
            m_running.store(false);
            Log::Error("Failed to bind WebRadar server to port " + std::to_string(m_port));
        }

        m_running.store(false);
    }
}
