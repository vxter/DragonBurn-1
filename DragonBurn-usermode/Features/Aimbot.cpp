#include "Aimbot.h"
#undef max()
#undef min()

void AimControl::switchToggle()
{
    LegitBotConfig::AimAlways = !LegitBotConfig::AimAlways;
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

void AimControl::AimBot(const CEntity& Local, Vec3 LocalPos,std::vector<Vec3>& AimPosList)
{
    if (MenuConfig::ShowMenu)
        return;

    std::string curWeapon = TriggerBot::GetWeapon(Local);
    if (!TriggerBot::CheckWeapon(curWeapon))
        return;

    if (onlyAuto && !CheckAutoMode(curWeapon))
        return;

    if (Local.Pawn.ShotsFired <= AimBullet - 1 && AimBullet != 0)
    {
        HasTarget = false;
        return;
    }

    if (AimControl::ScopeOnly)
    {
        bool isScoped;
        memoryManager.ReadMemory<bool>(Local.Pawn.Address + Offset.Pawn.isScoped, isScoped);
        if (!isScoped && TriggerBot::CheckScopeWeapon(curWeapon))
        {
            HasTarget = false;
            return;
        }
    }

    if (!IgnoreFlash && Local.Pawn.FlashDuration > 0.f)
        return;

    const int ListSize = AimPosList.size();
    if (ListSize == 0) {
        HasTarget = false;
        return;
    }

    float BestNorm = MAXV;
    int BestTargetIndex = -1;
    Vec2 Angles{ 0, 0 };

    const int ScreenCenterX = Gui.Window.Size.x / 2;
    const int ScreenCenterY = Gui.Window.Size.y / 2;

    for (int i = 0; i < ListSize; i++)
    {
        Vec3 OppPos = AimPosList[i] - LocalPos;
        const float Distance = sqrt(OppPos.x * OppPos.x + OppPos.y * OppPos.y);
        if (LegitBotConfig::RCS)
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
            AimPosList[i] = LocalPos + OppPos;
        }

        const float Yaw = atan2f(OppPos.y, OppPos.x) * 57.295779513f - Local.Pawn.ViewAngle.y;
        const float Pitch = -atan(OppPos.z / Distance) * 57.295779513f - Local.Pawn.ViewAngle.x;
        const float Norm = sqrt(Yaw * Yaw + Pitch * Pitch);

        if (Norm < BestNorm) {
            BestNorm = Norm;
            BestTargetIndex = i;
        }
    }

    if (BestNorm >= AimFov || BestNorm <= AimFovMin || BestTargetIndex == -1) {
        HasTarget = false;
        return;
    }

    Vec2 ScreenPos;
    if (!gGame.View.WorldToScreen(AimPosList[BestTargetIndex], ScreenPos)) {
        HasTarget = false;
        return;
    }

    HasTarget = true;

    auto [TargetX, TargetY] = CalculateTargetOffset(ScreenPos, ScreenCenterX, ScreenCenterY);

    TargetX /= Local.Client.Sensitivity /4;
    TargetY /= Local.Client.Sensitivity /4;
    if (Smooth > 0.0f)
    {
        const float DistanceRatio = BestNorm / AimFov;
        const float SpeedFactor = 1.0f + (1.0f - DistanceRatio);
        TargetX /= (Smooth * SpeedFactor);
        TargetY /= (Smooth * SpeedFactor);
    }

    if (HumanizeVar)
    {
        auto [HumanizedX, HumanizedY] = Humanize(TargetX, TargetY);
        TargetX = HumanizedX;
        TargetY = HumanizedY;
    }

    static DWORD lastAimTime = GetTickCount64();
    DWORD currentTick = GetTickCount64();

    if (currentTick - lastAimTime >= MenuConfig::AimDelay)
    {
        mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);
        lastAimTime = currentTick;
    }
}

bool AimControl::CheckAutoMode(const std::string& WeaponName)
{
    if (WeaponName == "deagle" || WeaponName == "elite" || WeaponName == "fiveseven" || WeaponName == "glock" || WeaponName == "awp" || WeaponName == "xm1014" || WeaponName == "mag7" || WeaponName == "sawedoff" || WeaponName == "tec9" || WeaponName == "zeus" || WeaponName == "p2000" || WeaponName == "nova" || WeaponName == "p250" || WeaponName == "ssg08" || WeaponName == "usp" || WeaponName == "revolver")
        return false;
    else
        return true;
}