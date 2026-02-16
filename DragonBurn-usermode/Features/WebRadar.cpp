#include "WebRadar.h"
#include "../Helpers/Logger.h"
#include "../Core/GlobalVars.h"
#include "../Core/MemoryMgr.h"
#include <sstream>
#include <iomanip>

namespace WebRadar
{
    // Helper function to get current map name
    static std::string GetCurrentMapName()
    {
        if (!g_globalVars || !g_globalVars->g_cCurrentMap)
            return "unknown";

        char currentMap[256] = { 0 };
        if (!memoryManager.ReadMemory(reinterpret_cast<DWORD64>(g_globalVars->g_cCurrentMap),
            currentMap, sizeof(currentMap) - 1))
        {
            return "unknown";
        }

        currentMap[255] = '\0';
        return std::string(currentMap);
    }
    WebRadarManager::WebRadarManager()
        : m_initialized(false)
        , m_lastMapName("")
    {
    }

    WebRadarManager::~WebRadarManager()
    {
        Shutdown();
    }

    bool WebRadarManager::Initialize(int port)
    {
        if (m_initialized)
        {
            Log::Warning("WebRadar already initialized");
            return true;
        }

        try
        {
            m_server = std::make_unique<WebServer>();
            
            if (!m_server->Start(port))
            {
                Log::Error("Failed to start WebRadar server");
                return false;
            }

            m_initialized = true;
            Log::Fine("WebRadar initialized successfully on port " + std::to_string(port));
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("WebRadar initialization exception: " + std::string(e.what()));
            return false;
        }
    }

    void WebRadarManager::Shutdown()
    {
        if (!m_initialized)
            return;

        if (m_server)
        {
            m_server->Stop();
            m_server.reset();
        }

        m_initialized = false;
        Log::Info("WebRadar shutdown");
    }

    void WebRadarManager::UpdateRadarData(
        const std::vector<std::pair<int, CEntity>>& entities,
        const CEntity& localEntity,
        int localPlayerControllerIndex,
        DWORD tickCount)
    {
        if (!m_initialized || !m_server || !m_server->IsRunning())
            return;

        // Collect player data
        std::vector<PlayerData> players;
        players.reserve(entities.size());

        for (const auto& [entityIndex, entity] : entities)
        {
            if (!entity.IsAlive())
                continue;

            PlayerData player;
            player.position = entity.Pawn.Pos;
            player.cameraPos = entity.Pawn.CameraPos;
            player.health = entity.Pawn.Health;
            player.teamId = entity.Pawn.TeamID;
            player.weaponName = std::string(entity.Pawn.WeaponName);
            player.playerName = std::string(entity.Controller.PlayerName);
            player.viewAngle = entity.Pawn.ViewAngle;
            player.entityIndex = entityIndex;

            players.push_back(player);
        }

        // Get current map name
        MapData mapData;
        mapData.name = GetCurrentMapName();
        mapData.scale = WebRadarCFG::MapScale;
        mapData.offsetX = WebRadarCFG::OffsetX;
        mapData.offsetY = WebRadarCFG::OffsetY;

        // Build and send JSON
        std::string json = BuildJSON(players, mapData, localEntity.Controller.TeamID, tickCount);
        m_server->UpdateRadarData(json);
    }

    bool WebRadarManager::IsEnabled() const
    {
        return m_initialized && m_server && m_server->IsRunning();
    }

    void WebRadarManager::SetEnabled(bool enabled)
    {
        if (enabled && !m_initialized)
        {
            Initialize(WebRadarCFG::Port);
        }
        else if (!enabled && m_initialized)
        {
            Shutdown();
        }
    }

    std::string WebRadarManager::BuildJSON(
        const std::vector<PlayerData>& players,
        const MapData& mapData,
        int localTeamId,
        DWORD tickCount)
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);

        json << "{";
        
        // Map data
        json << "\"map\":{";
        json << "\"name\":\"" << mapData.name << "\",";
        json << "\"scale\":" << mapData.scale << ",";
        json << "\"offsetX\":" << mapData.offsetX << ",";
        json << "\"offsetY\":" << mapData.offsetY;
        json << "},";

        // Local team ID
        json << "\"localTeamId\":" << localTeamId << ",";

        // Tick count
        json << "\"tickCount\":" << tickCount << ",";

        // Players array
        json << "\"players\":[";
        for (size_t i = 0; i < players.size(); ++i)
        {
            const auto& player = players[i];

            json << "{";
            json << "\"entityIndex\":" << player.entityIndex << ",";
            json << "\"position\":{";
            json << "\"x\":" << player.position.x << ",";
            json << "\"y\":" << player.position.y << ",";
            json << "\"z\":" << player.position.z;
            json << "},";
            json << "\"cameraPos\":{";
            json << "\"x\":" << player.cameraPos.x << ",";
            json << "\"y\":" << player.cameraPos.y << ",";
            json << "\"z\":" << player.cameraPos.z;
            json << "},";
            json << "\"viewAngle\":{";
            json << "\"x\":" << player.viewAngle.x << ",";
            json << "\"y\":" << player.viewAngle.y;
            json << "},";
            json << "\"health\":" << player.health << ",";
            json << "\"teamId\":" << player.teamId << ",";
            json << "\"weaponName\":\"" << player.weaponName << "\",";
            json << "\"name\":\"" << player.playerName << "\"";
            json << "}";

            if (i < players.size() - 1)
                json << ",";
        }
        json << "]";

        json << "}";

        return json.str();
    }
}
