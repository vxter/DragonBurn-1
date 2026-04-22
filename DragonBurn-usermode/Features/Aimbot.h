#pragma once
#define _USE_MATH_DEFINES
#define MAXV 10000e9
#include <math.h>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include <cstdint>
#include "..\Game\Game.h"
#include "..\Game\Entity.h"
#include "..\Core\Config.h"
#include <iostream>
#include "..\Game\View.h"
#include "..\Features/RCS.h"
#include "TriggerBot.h"
#include <random>

extern "C" {
#include "..\Helpers\Mouse.h"
#include "..\Game\Entity.h"
}


namespace AimControl
{
    inline int HotKey = VK_LBUTTON;
    inline int AimBullet = 1;
    inline bool ScopeOnly = true;
    inline bool IgnoreFlash = false;
    inline bool HumanizeVar = true;
    inline int HumanizationStrength = 5;
    inline float AimFov = 10;
    inline float AimFovMin = 0.0f;
    inline bool UseMinFovDeadzone = false;
    inline float Smooth = 5.0f;
    inline std::vector<int> HitboxList{ BONEINDEX::head };
    inline bool HasTarget = false;
    inline Vec2 LastTargetScreenPos{0,0};
    inline Vec3 LastTargetWorldPos{0,0,0};
    inline bool onlyAuto = false;
    inline bool UseEdgeSampling = true;
    inline float HeadOffset = 0.0f;
    inline float HeadDropOffset = 0.0f;

    enum class AimSampleKind : uint8_t
    {
        Bone,
        Body,
        Edge,
        Corner
    };

    struct AimDebugSample
    {
        Vec3 WorldPos;
        Vec2 ScreenPos;
        AimSampleKind Kind;
        int BoneIndex = -1;
        bool InsideFov = false;
        bool VisibilityOk = false;
        bool Accepted = false;
    };

    inline std::vector<AimDebugSample> DebugSamples;
    inline constexpr size_t DebugSampleLimit = 256;

    struct AimPoint
    {
        Vec3 WorldPos{ 0,0,0 };
        int DamageScore = 0;
        AimSampleKind Kind = AimSampleKind::Body;
        int BoneIndex = -1;
        Vec2 ScreenPos{ 0,0 };
        bool HasScreenPos = false;
        float ScreenDistSq = 0.f;
        float ScreenDistRatio = 0.f;
        bool InsideScreenFov = false;
        bool InsideScreenDeadzone = false;
    };

    static float PrevTargetX = 0.0f;
    static float PrevTargetY = 0.0f;
    static std::random_device rd;
    static std::mt19937 gen(rd());
   

    std::pair<float, float> Humanize(float TargetX, float TargetY);

    // AimPosList: per-candidate world positions with priority
    void AimBot(const CEntity& Local, Vec3 LocalPos, std::vector<AimPoint>& AimPosList);
    void switchToggle();
    std::pair<float, float> CalculateTargetOffset(const Vec2& ScreenPos, int ScreenCenterX, int ScreenCenterY);
    bool CheckAutoMode(const std::string& WeaponName);
    void ClearDebugSamples();
    void AddDebugSample(const Vec3& worldPos, const Vec2& screenPos, AimSampleKind kind,
        bool insideFov, bool visibilityOk, bool accepted, int boneIndex = -1);
}
