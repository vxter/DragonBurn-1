#include "TriggerBot.h"
#include <chrono>
#include <random>
#include <thread>
#include <algorithm>
#include <cmath>

#include "../Core/GlobalVars.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace
{
    float EstimateTickIntervalMs()
    {
        if (g_globalVars && g_globalVars->g_fIntervalPerTick > 0.0001f)
        {
            return std::clamp(g_globalVars->g_fIntervalPerTick * 1000.f, 1.f, 100.f);
        }
        return 15.625f;
    }

    bool ReadLatestAimPunch(const CEntity& entity, Vec2& outPunch)
    {
        const auto count = static_cast<DWORD>(entity.Pawn.AimPunchCache.Count);
        if (count > 0 && count < 0xFFFF && entity.Pawn.AimPunchCache.Data != 0)
        {
            Vec2 punchSample{};
            if (memoryManager.ReadMemory<Vec2>(entity.Pawn.AimPunchCache.Data + (count - 1) * sizeof(Vec3), punchSample))
            {
                outPunch = punchSample;
                return true;
            }
        }

        outPunch = entity.Pawn.AimPunchAngle;
        return true;
    }

    class AdaptiveDelayController
    {
    public:
        long long AdvanceTarget(const CEntity& entity)
        {
            const float dt = ConsumeDeltaMs();

            Vec2 punch{};
            ReadLatestAimPunch(entity, punch);
            const float magnitude = std::sqrt(punch.x * punch.x + punch.y * punch.y);

            float rising = 0.f;
            if (!magnitudeInitialized)
            {
                smoothedMagnitude = magnitude;
                previousMagnitude = magnitude;
                magnitudeInitialized = true;
            }
            else
            {
                const float recoveryWindow = std::max(1.f, static_cast<float>(TriggerBot::AdaptiveRecoveryMs));
                const float alpha = std::clamp(dt / recoveryWindow, 0.f, 1.f);
                smoothedMagnitude += alpha * (magnitude - smoothedMagnitude);
                rising = std::max(0.f, magnitude - previousMagnitude);
                previousMagnitude = magnitude;
            }

            const float suppressionHold = TriggerBot::AdaptiveSuppressionTicks * EstimateTickIntervalMs();
            if (entity.Pawn.WaitForNoAttack)
            {
                suppressionTimerMs = std::max(suppressionTimerMs, suppressionHold);
            }
            suppressionTimerMs = std::max(0.f, suppressionTimerMs - dt);

            float penalty = TriggerBot::AdaptiveRecoilScale * smoothedMagnitude +
                TriggerBot::AdaptiveDerivativeScale * rising +
                suppressionTimerMs;
            penalty = std::clamp(penalty, 0.f, static_cast<float>(TriggerBot::AdaptiveMaxExtraDelay));

            return static_cast<long long>(TriggerBot::TriggerDelay + penalty);
        }

        void AdvanceIdle()
        {
            const float dt = ConsumeDeltaMs();
            const float recoveryWindow = std::max(1.f, static_cast<float>(TriggerBot::AdaptiveRecoveryMs));
            const float decay = std::clamp(dt / recoveryWindow, 0.f, 1.f);
            smoothedMagnitude -= smoothedMagnitude * decay;
            previousMagnitude -= previousMagnitude * decay;
            if (smoothedMagnitude < 0.f)
                smoothedMagnitude = 0.f;
            if (previousMagnitude < 0.f)
                previousMagnitude = 0.f;
            suppressionTimerMs = std::max(0.f, suppressionTimerMs - dt);
        }

    private:
        float ConsumeDeltaMs()
        {
            const auto now = std::chrono::steady_clock::now();
            if (!timingInitialized)
            {
                timingInitialized = true;
                lastUpdate = now;
                return EstimateTickIntervalMs();
            }

            const float dt = std::chrono::duration<float, std::milli>(now - lastUpdate).count();
            lastUpdate = now;
            return std::clamp(dt, 1.f, 250.f);
        }

        float smoothedMagnitude = 0.f;
        float previousMagnitude = 0.f;
        float suppressionTimerMs = 0.f;
        std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();
        bool timingInitialized = false;
        bool magnitudeInitialized = false;
    };

    AdaptiveDelayController g_adaptiveDelay;

    struct AdaptiveDelayScope
    {
        bool consumed = false;

        long long Consume(const CEntity& entity)
        {
            consumed = true;
            if (!TriggerBot::AdaptiveDelay)
                return TriggerBot::TriggerDelay;
            return g_adaptiveDelay.AdvanceTarget(entity);
        }

        ~AdaptiveDelayScope()
        {
            if (consumed || !TriggerBot::AdaptiveDelay)
                return;
            g_adaptiveDelay.AdvanceIdle();
        }
    };
}

void TriggerBot::Run(const CEntity& LocalEntity, const int& LocalPlayerControllerIndex)
{
    AdaptiveDelayScope adaptiveScope;
    long long requiredDelayMs = TriggerDelay;

    if (MenuConfig::ShowMenu)
        return;

	// Don't rely on AliveStatus alone; after offset/overlay changes it can be stale.
	if (LocalEntity.Pawn.Address == 0 || LocalEntity.Pawn.Health <= 0)
		return;

    // Get the entity under the crosshair
    DWORD uHandle = 0;
    if (!memoryManager.ReadMemory<DWORD>(LocalEntity.Pawn.Address + Offset.Pawn.iIDEntIndex, uHandle))
    {
        g_HasValidTarget = false;
        g_TargetFoundTime = std::chrono::system_clock::now();
        return;
    }

    if (uHandle == -1)
    {
        g_HasValidTarget = false;
        g_TargetFoundTime = std::chrono::system_clock::now();
        return;
    }

    DWORD64 PawnAddress = CEntity::ResolveEntityHandle(uHandle);
    if (PawnAddress == 0)
    {
        g_HasValidTarget = false;
        g_TargetFoundTime = std::chrono::system_clock::now();
        return;
    }

    CEntity targetEntity;
    if (!targetEntity.UpdatePawn(PawnAddress))
    {
        g_HasValidTarget = false;
        g_TargetFoundTime = std::chrono::system_clock::now();
        return;
    }

    // Validate the targeted entity
    if (!CanTrigger(LocalEntity, targetEntity, LocalPlayerControllerIndex))
    {
        g_HasValidTarget = false;
        g_TargetFoundTime = std::chrono::system_clock::now();
        return;
    }

    if (!g_HasValidTarget)
    {
        g_TargetFoundTime = std::chrono::system_clock::now();
    }
    g_HasValidTarget = true;
    requiredDelayMs = adaptiveScope.Consume(LocalEntity);

    auto now = std::chrono::system_clock::now();

    // calculate elapsed time
    long long timeSinceLastShot = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - g_LastShotTime).count();
    long long timeSinceTargetFound = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - g_TargetFoundTime).count();

    // check conditions to shoot
    if ((GetAsyncKeyState(TriggerBot::HotKey) || LegitBotConfig::TriggerAlways) &&
        timeSinceLastShot >= ShotDuration &&
        timeSinceTargetFound >= requiredDelayMs)
    { ExecuteShot(); }
}

bool TriggerBot::CanTrigger(const CEntity& LocalEntity, const CEntity& TargetEntity, const int& LocalPlayerControllerIndex)
{
    // Check if target is in a valid state
    if (TargetEntity.Pawn.Address == 0)
        return false;

    // Check team
    if (MenuConfig::TeamCheck && LocalEntity.Pawn.TeamID == TargetEntity.Pawn.TeamID)
        return false;

	// Check if weapon is ready
	if (LocalEntity.Pawn.WaitForNoAttack)
		return false;

    // Check weapon type
    std::string currentWeapon = GetWeapon(LocalEntity);
    if (!CheckWeapon(currentWeapon))
        return false;

    //check is velocity == 0
    if(StopedOnly && LocalEntity.Pawn.Speed != 0)
        return false;

    // Check flash duration
    if (!IgnoreFlash && LocalEntity.Pawn.FlashDuration > 0.0f)
        return false;

	// Check TTD timout
	DWORD64 playerMask = (DWORD64(1) << LocalPlayerControllerIndex);
	bool bIsVisible = (TargetEntity.Pawn.bSpottedByMask & playerMask) || (LocalEntity.Pawn.bSpottedByMask & playerMask);
	if (TTDtimeout && !bIsVisible)
		return false;

	// If spotted masks are uninitialized (both 0) while VisibleCheck is enabled,
	// don't assume visibility. Treat as not visible.
	if (VisibleCheck && TargetEntity.Pawn.bSpottedByMask == 0 && LocalEntity.Pawn.bSpottedByMask == 0)
		return false;

    // Check scope requirement
	if (ScopeOnly && CheckScopeWeapon(currentWeapon))
	{
		bool isScoped = false;
		if (memoryManager.ReadMemory<bool>(LocalEntity.Pawn.Address + Offset.Pawn.isScoped, isScoped))
		{
			if (!isScoped)
				return false;
		}
	}

    return true;
}

void TriggerBot::ExecuteShot()
{
    // Check if already shooting to avoid double-click
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        return;

    // Update timing
    g_LastShotTime = std::chrono::system_clock::now();

    // Execute shot with random timing
    std::random_device RandomDevice;
    std::mt19937 RandomNumber(RandomDevice());
    std::uniform_int_distribution<> Range(1, 5);
    auto rand = std::chrono::microseconds(Range(RandomNumber));

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::microseconds(Range(RandomNumber)));
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

std::string TriggerBot::GetWeapon(const CEntity& LocalEntity)
{
    // Single memory read to get the weapon pointer
    DWORD64 CurrentWeapon;
    if (!memoryManager.ReadMemory(LocalEntity.Pawn.Address + Offset.Pawn.pClippingWeapon, CurrentWeapon) || CurrentWeapon == 0)
        return "";

    // Calculate the final address for weapon index directly
    DWORD64 weaponIndexAddress = CurrentWeapon + Offset.EconEntity.AttributeManager +
        Offset.WeaponBaseData.Item + Offset.WeaponBaseData.ItemDefinitionIndex;

    // Single memory read to get weapon index
    short weaponIndex;
    if (!memoryManager.ReadMemory(weaponIndexAddress, weaponIndex) || weaponIndex == -1)
        return "";

    // Inline weapon name lookup
    static const std::string defaultWeapon = "";
    auto it = CEntity::weaponNames.find(weaponIndex);
    return (it != CEntity::weaponNames.end()) ? it->second : defaultWeapon;
}

bool TriggerBot::CheckScopeWeapon(const std::string& WeaponName)
{
    return (WeaponName == "awp" || WeaponName == "g3Sg1" || WeaponName == "ssg08" || WeaponName == "scar20");
}

bool TriggerBot::CheckWeapon(const std::string& WeaponName)
{
    return !(WeaponName == "smokegrenade" || WeaponName == "flashbang" || WeaponName == "hegrenade" ||
        WeaponName == "molotov" || WeaponName == "decoy" || WeaponName == "incgrenade" ||
        WeaponName == "t_knife" || WeaponName == "ct_knife" || WeaponName == "c4");
}
