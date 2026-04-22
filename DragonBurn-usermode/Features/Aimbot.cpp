#include "Aimbot.h"
#undef max()
#undef min()

#include <cmath>
#include <algorithm>
#include <limits>

void AimControl::switchToggle()
{
    LegitBotConfig::AimAlways = !LegitBotConfig::AimAlways;
}

void AimControl::ClearDebugSamples()
{
    DebugSamples.clear();
}

void AimControl::AddDebugSample(const Vec3& worldPos, const Vec2& screenPos, AimSampleKind kind,
    bool insideFov, bool visibilityOk, bool accepted, int boneIndex)
{
    if (!ESPConfig::ShowAimSamples)
        return;
    if (DebugSamples.size() >= DebugSampleLimit)
        return;
    DebugSamples.push_back({ worldPos, screenPos, kind, boneIndex, insideFov, visibilityOk, accepted });
}

std::pair<float, float> AimControl::CalculateTargetOffset(const Vec2& ScreenPos, int ScreenCenterX, int ScreenCenterY)
{
    float TargetX = 0.0f;
    float TargetY = 0.0f;

    /*x*/
    if (ScreenPos.x != ScreenCenterX) {
        TargetX = (ScreenPos.x > ScreenCenterX) ?
            -(ScreenCenterX - ScreenPos.x) :
            ScreenPos.x - ScreenCenterX;

        if (TargetX + ScreenCenterX > ScreenCenterX * 2 || TargetX + ScreenCenterX < 0) {
            TargetX = 0.0f;
        }
    }

    /*y*/
    if (ScreenPos.y != 0 && ScreenPos.y != ScreenCenterY) {
        TargetY = (ScreenPos.y > ScreenCenterY) ?
            -(ScreenCenterY - ScreenPos.y) :
            ScreenPos.y - ScreenCenterY;

        if (TargetY + ScreenCenterY > ScreenCenterY * 2 || TargetY + ScreenCenterY < 0) {
            TargetY = 0.0f;
        }
    }

    return { TargetX, TargetY };
}

std::pair<float, float> AimControl::Humanize(float TargetX, float TargetY) {
    float HumanizationAmount = HumanizationStrength * 2 / 100;

    if (HumanizationAmount <= 0.0f) {
        PrevTargetX = TargetX;
        PrevTargetY = TargetY;
        return { TargetX, TargetY };
    }

    std::uniform_real_distribution<float> jitterDist(-1.f, 1.f);
    float JitterX = jitterDist(gen) * HumanizationAmount;
    float JitterY = jitterDist(gen) * HumanizationAmount;

    float SmoothFactor = 1.0f - HumanizationAmount * 0.5f;
    float SmoothedX = TargetX * SmoothFactor + PrevTargetX * (1.0f - SmoothFactor);
    float SmoothedY = TargetY * SmoothFactor + PrevTargetY * (1.0f - SmoothFactor);

    PrevTargetX = TargetX;
    PrevTargetY = TargetY;

    return { SmoothedX + JitterX, SmoothedY + JitterY };
}

void AimControl::AimBot(const CEntity& Local, Vec3 LocalPos, std::vector<AimPoint>& AimPosList)
{
    if (MenuConfig::ShowMenu)
        return;

    std::string curWeapon = TriggerBot::GetWeapon(Local);
    if (!TriggerBot::CheckWeapon(curWeapon))
        return;

    if (onlyAuto && !CheckAutoMode(curWeapon))
        return;

    // If ShotsFired reads as 0/uninitialized, don't block aiming.
    if (AimBullet != 0 && Local.Pawn.ShotsFired != 0 && Local.Pawn.ShotsFired <= AimBullet - 1)
    {
        HasTarget = false;
        return;
    }

	if (AimControl::ScopeOnly)
	{
		bool isScoped;
		if (memoryManager.ReadMemory<bool>(Local.Pawn.Address + Offset.Pawn.isScoped, isScoped))
		{
			if (!isScoped && TriggerBot::CheckScopeWeapon(curWeapon))
			{
				HasTarget = false;
				return;
			}
		}
	}

	// Only treat flash as blocking if it's plausibly initialized.
	if (!IgnoreFlash)
	{
		if (Local.Pawn.FlashDuration > 0.f && Local.Pawn.FlashDuration < 5.0f)
			return;
	}

    if (AimControl::HitboxList.empty()) {
        HasTarget = false;
        return;
    }

    const int ListSize = static_cast<int>(AimPosList.size());
    if (ListSize == 0) {
        HasTarget = false;
        return;
    }

    static ULONGLONG lastAimDbg = 0;
	if (!std::isfinite(LocalPos.x) || !std::isfinite(LocalPos.y) || !std::isfinite(LocalPos.z))
	{
		if (GetTickCount64() - lastAimDbg > 3000)
		{
			lastAimDbg = GetTickCount64();
			std::printf("[AimDbg] LocalPos non-finite: (%.3f,%.3f,%.3f)\n", LocalPos.x, LocalPos.y, LocalPos.z);
		}
		HasTarget = false;
		return;
	}

    float BestNorm = MAXV;
    float BestRawNorm = MAXV;
    int BestTargetIndex = -1;
    int BestDamageScore = std::numeric_limits<int>::min();
    Vec2 Angles{ 0, 0 };
	int invalidTargets = 0;
	int invalidOpp = 0;
	int invalidDistance = 0;
	int invalidNorm = 0;

	const int ScreenCenterX = Gui.Window.Size.x / 2;
	const int ScreenCenterY = Gui.Window.Size.y / 2;
	const bool enforceAimFov = AimControl::AimFov > 0.01f;
	const float maxAimFov = enforceAimFov ? std::clamp(AimControl::AimFov, 0.1f, 179.f) : 0.f;
    const float minAimFov = (enforceAimFov && AimControl::AimFovMin > 0.01f)
        ? std::min(AimControl::AimFovMin, maxAimFov)
        : 0.f;
    const bool enforceDeadzone = AimControl::UseMinFovDeadzone && minAimFov > 0.f;

	for (int i = 0; i < ListSize; ++i)
	{
        Vec3 OppPos = AimPosList[i].WorldPos - LocalPos;
		if (!std::isfinite(OppPos.x) || !std::isfinite(OppPos.y) || !std::isfinite(OppPos.z))
		{
			++invalidTargets;
			++invalidOpp;
			continue;
		}

        const float DistanceRaw = sqrt(OppPos.x * OppPos.x + OppPos.y * OppPos.y);
        if (!std::isfinite(DistanceRaw))
        {
            ++invalidTargets;
            ++invalidDistance;
            continue;
        }

        // Close-range stability: avoid division by ~0 in recoil math.
        // If we are almost aligned vertically (very small horizontal distance),
        // recoil rotation becomes numerically unstable and can push the point
        // behind the camera. In that case, skip recoil rotation for this candidate.
        constexpr float DIST_EPS = 1e-3f;
        const bool skipRcsRotation = DistanceRaw < DIST_EPS;
        const float Distance = skipRcsRotation ? DIST_EPS : DistanceRaw;
        if (!skipRcsRotation && LegitBotConfig::RCS && !RCS::IsCalibrating() && Local.Pawn.ShotsFired > static_cast<DWORD>(RCS::RCSBullet))
        {
            Vec2 profileAimPunch{ 0.f,0.f };
            const bool hasProfile = RCS::GetProfileAimPunch(Local, profileAimPunch);

            if (hasProfile)
            {
					// Profile is stored as aimPunchAngle per shot. Apply it directly (no generic RCS scaling).
					Angles = profileAimPunch;

					// Match clamping/normalization behavior of RCS::UpdateAngles.
					if (Angles.x > 89.f) Angles.x = 89.f;
					if (Angles.x < -89.f) Angles.x = -89.f;
					while (Angles.y > 180.f) Angles.y -= 360.f;
					while (Angles.y < -180.f) Angles.y += 360.f;

					/*x*/
					const float radX = Angles.x * RCS::RCSScale.x / 360.f * M_PI;
                const float sinX = sinf(radX);
                const float cosX = cosf(radX);

                const float z = OppPos.z * cosX + Distance * sinX;
                const float d = (Distance * cosX - OppPos.z * sinX) / Distance;

					/*y*/
					const float radY = -Angles.y * RCS::RCSScale.y / 360.f * M_PI;
                const float sinY = sinf(radY);
                const float cosY = cosf(radY);

                const float x = (OppPos.x * cosY - OppPos.y * sinY) * d;
                const float y = (OppPos.x * sinY + OppPos.y * cosY) * d;

                OppPos = Vec3{ x, y, z };
                AimPosList[i].WorldPos = LocalPos + OppPos;
            }
            else
            {
                RCS::UpdateAngles(Local, Angles);

                /*x*/
                const float radX = Angles.x * RCS::RCSScale.x / 360.f * M_PI;
                const float sinX = sinf(radX);
                const float cosX = cosf(radX);

                const float z = OppPos.z * cosX + Distance * sinX;
                const float d = (Distance * cosX - OppPos.z * sinX) / Distance;

                /*y*/
                const float radY = -Angles.y * RCS::RCSScale.y / 360.f * M_PI;
                const float sinY = sinf(radY);
                const float cosY = cosf(radY);

                const float x = (OppPos.x * cosY - OppPos.y * sinY) * d;
                const float y = (OppPos.x * sinY + OppPos.y * cosY) * d;

                OppPos = Vec3{ x, y, z };
                AimPosList[i].WorldPos = LocalPos + OppPos;
            }
        }

        const float Yaw = atan2f(OppPos.y, OppPos.x) * 57.295779513f - Local.Pawn.ViewAngle.y;
        // Use atan2f to avoid divide-by-zero artifacts.
        const float Pitch = -atan2f(OppPos.z, Distance) * 57.295779513f - Local.Pawn.ViewAngle.x;
        const float Norm = sqrt(Yaw * Yaw + Pitch * Pitch);
		if (!std::isfinite(Norm))
		{
			++invalidTargets;
			++invalidNorm;
			continue;
		}
        const bool hasScreen = AimPosList[i].HasScreenPos;
        const bool insideScreenFov = hasScreen ? AimPosList[i].InsideScreenFov : false;
        const bool insideScreenDeadzone = hasScreen ? AimPosList[i].InsideScreenDeadzone : false;

        if (enforceAimFov)
        {
            if (hasScreen)
            {
                if (!insideScreenFov)
                    continue;
                if (enforceDeadzone && insideScreenDeadzone)
                    continue;
            }
            else
            {
                if (Norm > maxAimFov)
                    continue;
                if (enforceDeadzone && Norm < minAimFov)
                    continue;
            }
        }
        else if (enforceDeadzone && hasScreen && insideScreenDeadzone)
        {
            continue;
        }

        float evalNorm = Norm;
        if (hasScreen && enforceAimFov && maxAimFov > 0.0f)
        {
            const float ratio = std::clamp(AimPosList[i].ScreenDistRatio, 0.f, 1.f);
            evalNorm = ratio * maxAimFov;
        }
        if (!enforceDeadzone && minAimFov > 0.f && evalNorm < minAimFov)
            evalNorm = minAimFov;

        const int candidateDamage = AimPosList[i].DamageScore;
        if (candidateDamage > BestDamageScore || (candidateDamage == BestDamageScore && evalNorm < BestNorm)) {
            BestDamageScore = candidateDamage;
            BestNorm = evalNorm;
            BestRawNorm = Norm;
            BestTargetIndex = i;
        }
    }

    // only abort if no valid target
    if (BestTargetIndex == -1) {
        HasTarget = false;
        return;
    }

    Vec2 ScreenPos;
    if (!gGame.View.WorldToScreen(AimPosList[BestTargetIndex].WorldPos, ScreenPos)) {
        HasTarget = false;
		if (GetTickCount64() - lastAimDbg > 3000)
		{
			lastAimDbg = GetTickCount64();
			std::printf("[AimDbg] abort W2S failed idx=%d\n", BestTargetIndex);
		}
        return;
    }

    HasTarget = true;
    // cache the selected positions for visualization
    LastTargetWorldPos = AimPosList[BestTargetIndex].WorldPos;
    LastTargetScreenPos = ScreenPos;

    auto [rawOffsetX, rawOffsetY] = CalculateTargetOffset(ScreenPos, ScreenCenterX, ScreenCenterY);
    float TargetX = rawOffsetX;
    float TargetY = rawOffsetY;

    TargetX /= Local.Client.Sensitivity / 4;
    TargetY /= Local.Client.Sensitivity / 4;
	const float smoothingReference = enforceAimFov ? maxAimFov : std::max(BestNorm, 0.1f);
	if (Smooth > 0.0f && smoothingReference > 0.0f)
	{
		const float normalizedNorm = std::min(BestNorm, smoothingReference) / smoothingReference;
		const float SpeedFactor = 1.0f + (1.0f - normalizedNorm);
		TargetX /= (Smooth * SpeedFactor);
		TargetY /= (Smooth * SpeedFactor);
	}

    if (HumanizeVar)
    {
        auto [HumanizedX, HumanizedY] = Humanize(TargetX, TargetY);
        TargetX = HumanizedX;
        TargetY = HumanizedY;
    }

    // clamp overshoot: prevent exceeding raw movement
    float rawTargetX = rawOffsetX / (Local.Client.Sensitivity / 4);
    float rawTargetY = rawOffsetY / (Local.Client.Sensitivity / 4);
    if ((TargetX > 0 && TargetX > rawTargetX) || (TargetX < 0 && TargetX < rawTargetX))
        TargetX = rawTargetX;
    if ((TargetY > 0 && TargetY > rawTargetY) || (TargetY < 0 && TargetY < rawTargetY))
        TargetY = rawTargetY;

    static DWORD lastAimTime = GetTickCount64();
    DWORD currentTick = GetTickCount64();

    if (currentTick - lastAimTime >= MenuConfig::AimDelay)
    {
        mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);
        lastAimTime = currentTick;
		if (currentTick - lastAimDbg > 3000)
		{
			lastAimDbg = currentTick;
			std::printf("[AimDbg] move TargetX=%.2f TargetY=%.2f idx=%d\n", TargetX, TargetY, BestTargetIndex);
		}
    }
}

bool AimControl::CheckAutoMode(const std::string& WeaponName)
{
    if (WeaponName == "deagle" || WeaponName == "elite" || WeaponName == "fiveseven" || WeaponName == "glock" || WeaponName == "awp" || WeaponName == "xm1014" || WeaponName == "mag7" || WeaponName == "sawedoff" || WeaponName == "tec9" || WeaponName == "zeus" || WeaponName == "p2000" || WeaponName == "nova" || WeaponName == "p250" || WeaponName == "ssg08" || WeaponName == "usp" || WeaponName == "revolver")
        return false;
    else
        return true;
}
// map bone ID to string name
