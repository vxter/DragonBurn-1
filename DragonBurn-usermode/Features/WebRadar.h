#pragma once
#include "../Game/Entity.h"
#include "../WebServer/WebServer.h"
#include <memory>
#include <string>
#include <vector>

namespace WebRadarCFG
{
    inline bool Enabled = false;
    inline int Port = 8080;
    inline float MapScale = 4.0f;
    inline float OffsetX = 0.0f;
    inline float OffsetY = 0.0f;
}

namespace WebRadar
{
    struct PlayerData
    {
        Vec3 position;
        Vec3 cameraPos;
        int health;
        int teamId;
        std::string weaponName;
        std::string playerName;
        Vec2 viewAngle;
        int entityIndex;
        bool isLocalPlayer;
    };

    struct MapData
    {
        std::string name;
        float scale;
        float offsetX;
        float offsetY;
    };

    struct BombData
    {
        Vec3 position;
        bool isPlanted;
        int bombSite;  // 0=A, 1=B
        bool isBeingDefused;
    };

    class WebRadarManager
    {
    public:
        WebRadarManager();
        ~WebRadarManager();

        bool Initialize(int port = 8080);
        void Shutdown();
        
        void UpdateRadarData(
            const std::vector<std::pair<int, CEntity>>& entities,
            const CEntity& localEntity,
            int localPlayerControllerIndex,
            DWORD tickCount,
            const BombData* bombData = nullptr);

        bool IsEnabled() const;
        void SetEnabled(bool enabled);

    private:
        std::string BuildJSON(
            const std::vector<PlayerData>& players,
            const MapData& mapData,
            int localTeamId,
            DWORD tickCount,
            int localPlayerIndex,
            const BombData* bombData = nullptr);

        std::unique_ptr<WebServer> m_server;
        bool m_initialized;
        std::string m_lastMapName;
    };

    inline std::unique_ptr<WebRadarManager> g_webRadar = nullptr;
}
