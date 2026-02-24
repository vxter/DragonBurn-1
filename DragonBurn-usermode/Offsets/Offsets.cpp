#include "Offsets.h"
#include "../Core/Init.h"
#include "../Helpers/StorageMgr.h"
#include "../Helpers/Logger.h"

Offsets::Offsets() {}

Offsets::~Offsets() {}

void Offsets::SetOffsets(const std::string& offsetsData, const std::string& buttonsData, const std::string& client_dllData)
{
    try {
        json offsetsJson = json::parse(offsetsData);
        json buttonsJson = json::parse(buttonsData);
        json client_dllJson = json::parse(client_dllData)["client.dll"]["classes"];

        Log::Fine("Setting basic offsets...");
        this->EntityList = offsetsJson["client.dll"]["dwEntityList"];
        this->Matrix = offsetsJson["client.dll"]["dwViewMatrix"];
        this->ViewAngle = offsetsJson["client.dll"]["dwViewAngles"];
        this->LocalPlayerController = offsetsJson["client.dll"]["dwLocalPlayerController"];
        this->LocalPlayerPawn = offsetsJson["client.dll"]["dwLocalPlayerPawn"];
        this->GlobalVars = offsetsJson["client.dll"]["dwGlobalVars"];
        this->PlantedC4 = offsetsJson["client.dll"]["dwPlantedC4"];
        this->InputSystem = offsetsJson["inputsystem.dll"]["dwInputSystem"];
        this->Sensitivity = offsetsJson["client.dll"]["dwSensitivity"];
        this->Sensitivity_sensitivity = offsetsJson["client.dll"]["dwSensitivity_sensitivity"];

        Log::Fine("Setting button offsets...");
        this->Buttons.Attack = buttonsJson["client.dll"]["attack"];
        this->Buttons.Jump = buttonsJson["client.dll"]["jump"];
        this->Buttons.Right = buttonsJson["client.dll"]["right"];
        this->Buttons.Left = buttonsJson["client.dll"]["left"];

    Log::Fine("Setting entity offsets...");
    this->Entity.IsAlive = client_dllJson["CCSPlayerController"]["fields"]["m_bPawnIsAlive"];
    this->Entity.PlayerPawn = client_dllJson["CCSPlayerController"]["fields"]["m_hPlayerPawn"];
    this->Entity.iszPlayerName = client_dllJson["CBasePlayerController"]["fields"]["m_iszPlayerName"];

    Log::Fine("Setting pawn offsets...");
    
    Log::Fine("  m_pBulletServices");
    this->Pawn.BulletServices = client_dllJson["C_CSPlayerPawn"]["fields"]["m_pBulletServices"];
    
    Log::Fine("  m_pCameraServices");
    this->Pawn.CameraServices = client_dllJson["C_BasePlayerPawn"]["fields"]["m_pCameraServices"];
    
    Log::Fine("  m_pClippingWeapon");
    this->Pawn.pClippingWeapon = client_dllJson["C_CSPlayerPawn"]["fields"]["m_pClippingWeapon"];
    
    Log::Fine("  m_bIsScoped");
    this->Pawn.isScoped = client_dllJson["C_CSPlayerPawn"]["fields"]["m_bIsScoped"];
    
    Log::Fine("  m_bIsDefusing");
    this->Pawn.isDefusing = client_dllJson["C_CSPlayerPawn"]["fields"]["m_bIsDefusing"];
    
    Log::Fine("  m_totalHitsOnServer");
    this->Pawn.TotalHit = client_dllJson["CCSPlayer_BulletServices"]["fields"]["m_totalHitsOnServer"];
    
    Log::Fine("  m_vOldOrigin");
    this->Pawn.Pos = client_dllJson["C_BasePlayerPawn"]["fields"]["m_vOldOrigin"];
    
    Log::Fine("  m_ArmorValue");
    this->Pawn.CurrentArmor = client_dllJson["C_CSPlayerPawn"]["fields"]["m_ArmorValue"];
    
    Log::Fine("  m_iMaxHealth");
    this->Pawn.MaxHealth = client_dllJson["C_BaseEntity"]["fields"]["m_iMaxHealth"];
    
    Log::Fine("  m_iHealth");
    this->Pawn.CurrentHealth = client_dllJson["C_BaseEntity"]["fields"]["m_iHealth"];
    
    Log::Fine("  m_pGameSceneNode");
    this->Pawn.GameSceneNode = client_dllJson["C_BaseEntity"]["fields"]["m_pGameSceneNode"];
    
    Log::Fine("  m_modelState");
    this->Pawn.BoneArray = client_dllJson["CSkeletonInstance"]["fields"]["m_modelState"] + 0x80;
    
    Log::Fine("  m_angEyeAngles");
    this->Pawn.angEyeAngles = client_dllJson["C_CSPlayerPawn"]["fields"]["m_angEyeAngles"];
    
    Log::Fine("  m_vecLastClipCameraPos");
    this->Pawn.vecLastClipCameraPos = client_dllJson["C_CSPlayerPawn"]["fields"]["m_vecLastClipCameraPos"];
    
    Log::Fine("  m_iShotsFired");
    this->Pawn.iShotsFired = client_dllJson["C_CSPlayerPawn"]["fields"]["m_iShotsFired"];
    
    Log::Fine("  m_flFlashDuration");
    this->Pawn.flFlashDuration = client_dllJson["C_CSPlayerPawnBase"]["fields"]["m_flFlashDuration"];
    
    Log::Fine("  m_aimPunchAngle");
    this->Pawn.aimPunchAngle = client_dllJson["C_CSPlayerPawn"]["fields"]["m_aimPunchAngle"];
    
    Log::Fine("  m_aimPunchCache");
    if (client_dllJson["C_CSPlayerPawn"]["fields"].contains("m_aimPunchCache") && 
        !client_dllJson["C_CSPlayerPawn"]["fields"]["m_aimPunchCache"].is_null()) {
        this->Pawn.aimPunchCache = client_dllJson["C_CSPlayerPawn"]["fields"]["m_aimPunchCache"];
    } else {
        Log::Warning("  m_aimPunchCache is null, using default value 0");
        this->Pawn.aimPunchCache = 0;
    }
    
    Log::Fine("  m_iIDEntIndex");
    this->Pawn.iIDEntIndex = client_dllJson["C_CSPlayerPawn"]["fields"]["m_iIDEntIndex"];
    
    Log::Fine("  m_iTeamNum");
    this->Pawn.iTeamNum = client_dllJson["C_BaseEntity"]["fields"]["m_iTeamNum"];
    
    Log::Fine("  m_iFOVStart");
    this->Pawn.iFovStart = client_dllJson["CCSPlayerBase_CameraServices"]["fields"]["m_iFOVStart"];
    
    Log::Fine("  m_fFlags");
    this->Pawn.fFlags = client_dllJson["C_BaseEntity"]["fields"]["m_fFlags"];
    
    Log::Fine("  m_entitySpottedState + m_bSpottedByMask");
    this->Pawn.bSpottedByMask = DWORD(client_dllJson["C_CSPlayerPawn"]["fields"]["m_entitySpottedState"]) + DWORD(client_dllJson["EntitySpottedState_t"]["fields"]["m_bSpottedByMask"]);
    
    Log::Fine("  m_vecAbsVelocity");
    this->Pawn.AbsVelocity = client_dllJson["C_BaseEntity"]["fields"]["m_vecAbsVelocity"];
    
    Log::Fine("  m_bWaitForNoAttack");
    this->Pawn.m_bWaitForNoAttack = client_dllJson["C_CSPlayerPawn"]["fields"]["m_bWaitForNoAttack"];
    
    Log::Fine("  m_pWeaponServices");
    this->Pawn.m_pWeaponServices = client_dllJson["C_BasePlayerPawn"]["fields"]["m_pWeaponServices"];
    
    Log::Fine("  m_flEmitSoundTime");
    this->Pawn.m_flEmitSoundTime = client_dllJson["C_CSPlayerPawn"]["fields"]["m_flEmitSoundTime"];

    this->GlobalVar.RealTime = 0x00;
    this->GlobalVar.FrameCount = 0x04;
    this->GlobalVar.MaxClients = 0x10;
    this->GlobalVar.IntervalPerTick = 0x14;
    this->GlobalVar.CurrentTime = 0x30;
    this->GlobalVar.CurrentTime2 = 0x38;
    this->GlobalVar.TickCount = 0x48;
    this->GlobalVar.IntervalPerTick2 = 0x44;
    this->GlobalVar.CurrentNetchan = 0x0048;
    this->GlobalVar.CurrentMap = 0x0180;
    this->GlobalVar.CurrentMapName = 0x0188;

    Log::Fine("Setting player controller offsets...");
    this->PlayerController.m_nTickBase = client_dllJson["CBasePlayerController"]["fields"]["m_nTickBase"];
    this->PlayerController.m_steamID = client_dllJson["CBasePlayerController"]["fields"]["m_steamID"];
    this->PlayerController.m_hPawn = client_dllJson["CBasePlayerController"]["fields"]["m_hPawn"];
    this->PlayerController.m_pObserverServices = client_dllJson["C_BasePlayerPawn"]["fields"]["m_pObserverServices"];
    this->PlayerController.m_hObserverTarget = client_dllJson["CPlayer_ObserverServices"]["fields"]["m_hObserverTarget"];
    this->PlayerController.m_hController = client_dllJson["C_BasePlayerPawn"]["fields"]["m_hController"];
    this->PlayerController.PawnArmor = client_dllJson["CCSPlayerController"]["fields"]["m_iPawnArmor"];
    this->PlayerController.HasDefuser = client_dllJson["CCSPlayerController"]["fields"]["m_bPawnHasDefuser"];
    this->PlayerController.HasHelmet = client_dllJson["CCSPlayerController"]["fields"]["m_bPawnHasHelmet"];

    Log::Fine("Setting econ entity offsets...");
    this->EconEntity.AttributeManager = client_dllJson["C_EconEntity"]["fields"]["m_AttributeManager"];

    Log::Fine("Setting weapon offsets...");
    this->WeaponBaseData.WeaponDataPTR = DWORD(client_dllJson["C_BaseEntity"]["fields"]["m_nSubclassID"]) + 0x08;
    this->WeaponBaseData.szName = client_dllJson["CCSWeaponBaseVData"]["fields"]["m_szName"];
    this->WeaponBaseData.Clip1 = client_dllJson["C_BasePlayerWeapon"]["fields"]["m_iClip1"];
    this->WeaponBaseData.MaxClip = client_dllJson["CBasePlayerWeaponVData"]["fields"]["m_iMaxClip1"];
    this->WeaponBaseData.Item = client_dllJson["C_AttributeContainer"]["fields"]["m_Item"];
    this->WeaponBaseData.ItemDefinitionIndex = client_dllJson["C_EconItemView"]["fields"]["m_iItemDefinitionIndex"];
    this->WeaponBaseData.hMyWeapons = client_dllJson["CPlayer_WeaponServices"]["fields"]["m_hMyWeapons"];

    Log::Fine("Setting C4 offsets...");
    this->C4.m_bBeingDefused = client_dllJson["C_PlantedC4"]["fields"]["m_bBeingDefused"];
    this->C4.m_flDefuseCountDown = client_dllJson["C_PlantedC4"]["fields"]["m_flDefuseCountDown"];
    this->C4.m_nBombSite = client_dllJson["C_PlantedC4"]["fields"]["m_nBombSite"];

    Log::Fine("All offsets set successfully!");
    } catch (const json::exception& e) {
        Log::Error(std::string("JSON error in SetOffsets: ") + e.what());
    } catch (const std::exception& e) {
        Log::Error(std::string("Error in SetOffsets: ") + e.what());
    }
}

void Offsets::UpdateOffsets()
{
    Log::Fine("=== Starting UpdateOffsets ===");

    std::string offsets, buttons, client_dll;

    json GamaDataStorage;
    try
    {
        Log::Fine("Attempting to read gamedata.json");
        GamaDataStorage = json::parse(storage::ReadStorageFile("gamedata.json"));
        Log::Fine("gamedata.json loaded successfully");
    }
    catch(...)
    {
        Log::Warning("gamedata.json not found or invalid, creating new");
        GamaDataStorage = json::object();
    }

    try
    {
        Log::Fine("Attempting to read offsets.json");
        offsets = storage::ReadStorageFile("offsets.json");
        Log::Fine("offsets.json loaded successfully");

        Log::Fine("Attempting to read buttons.json");
        buttons = storage::ReadStorageFile("buttons.json");
        Log::Fine("buttons.json loaded successfully");

        Log::Fine("Attempting to read client_dll.json");
        client_dll = storage::ReadStorageFile("client_dll.json");
        Log::Fine("client_dll.json loaded successfully");
    }
    catch (...)
    {
        Log::Warning("Storage files not found, downloading from GitHub (a2x/cs2-dumper)");
        offsets = Web::Get("https://raw.githubusercontent.com/some-random-guy-ah/cs2-offsets/main/offsets.json");
        buttons = Web::Get("https://raw.githubusercontent.com/some-random-guy-ah/cs2-offsets/main/buttons.json");
        client_dll = Web::Get("https://raw.githubusercontent.com/some-random-guy-ah/cs2-offsets/main/client_dll.json");

        Log::Fine("Downloading successful, saving to storage");
        storage::WriteStorageFile("offsets.json", offsets);
        storage::WriteStorageFile("buttons.json", buttons);
        storage::WriteStorageFile("client_dll.json", client_dll);

        Log::Fine("Saving gamedata.json");
        GamaDataStorage["build-number"] = "latest";
        storage::WriteStorageFile("gamedata.json", GamaDataStorage.dump(4));
    }

    Log::Fine("=== Calling SetOffsets ===");
    SetOffsets(offsets, buttons, client_dll);
    Log::Fine("=== UpdateOffsets Complete ===");
}
