//______                            ______                  
//|  _  \                           | ___ \                 
//| | | |_ __ __ _  __ _  ___  _ __ | |_/ /_   _ _ __ _ __  
//| | | | '__/ _` |/ _` |/ _ \| '_ \| ___ \ | | | '__| '_ \ 
//| |/ /| | | (_| | (_| | (_) | | | | |_/ / |_| | |  | | | |
//|___/ |_|  \__,_|\__, |\___/|_| |_\____/ \__,_|_|  |_| |_|
//                  __/ |                                   
//                 |___/                                    
//
//https://discord.gg/5WcvdzFybD
//https://github.com/ByteCorum/DragonBurn

#include <string>
#include <thread>
#include <future>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <unordered_map>

#include "Cheats.h"
#include "Render.h"
#include "../Game/Bone.h"  // for BONEINDEX enum
#include "../Core/Config.h"

#include "../Core/Init.h"

#include "../Features/ESP.h"
#include "../Core/GUI.h"
#include "../Features/RCS.H"
#include "../Features/BombTimer.h"
#include "../Features/SpectatorList.h"
#include "../Helpers/Logger.h"
#include "../Features/SoundESP.h"
#include "../Features/WebRadar.h"

namespace
{
	inline Vec3 CrossProduct(const Vec3& a, const Vec3& b)
	{
		return Vec3{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
	}

	inline float LengthSquared(const Vec3& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	const char* BoneFriendlyName(size_t idx)
	{
		switch (idx)
		{
		case 0: return "root";
		case BONEINDEX::pelvis: return "pelvis";
		case BONEINDEX::spine_0: return "spine_0";
		case BONEINDEX::spine_1: return "spine_1";
		case BONEINDEX::spine_2: return "spine_2";
		case BONEINDEX::spine_3: return "spine_3";
		case BONEINDEX::neck_0: return "neck_0";
		case BONEINDEX::head: return "head";
		case 8: return "arm_upper_L";
		case 9: return "arm_lower_L";
		case BONEINDEX::hand_L: return "hand_L";
		case 12: return "clavicle_R";
		case BONEINDEX::arm_upper_R: return "arm_upper_R";
		case BONEINDEX::arm_lower_R: return "arm_lower_R";
		case BONEINDEX::hand_R: return "hand_R";
		case BONEINDEX::leg_upper_L: return "leg_upper_L";
		case BONEINDEX::leg_lower_L: return "leg_lower_L";
		case BONEINDEX::ankle_L: return "ankle_L";
		case BONEINDEX::leg_upper_R: return "leg_upper_R";
		case BONEINDEX::leg_lower_R: return "leg_lower_R";
		case BONEINDEX::ankle_R: return "ankle_R";
		case 29: return "leg_l_offset";
		case 30: return "leg_l_iktarget";
		case 31: return "leg_r_offset";
		case 32: return "leg_r_iktarget";
		case 33: return "eyeball_L";
		case 34: return "eyeball_R";
		case 35: return "eye_target";
		case 36: return "head_twist";
		case 37: return "finger_mid_meta_L";
		case 38: return "finger_mid_0_L";
		case 39: return "finger_mid_1_L";
		case 40: return "finger_mid_2_L";
		case 41: return "finger_pinky_meta_L";
		case 42: return "finger_pinky_0_L";
		case 43: return "finger_pinky_1_L";
		case 44: return "finger_pinky_2_L";
		case 45: return "finger_index_meta_L";
		case 46: return "finger_index_0_L";
		case 47: return "finger_index_1_L";
		case 48: return "finger_index_2_L";
		case 49: return "finger_thumb_0_L";
		case 50: return "finger_thumb_1_L";
		case 51: return "finger_thumb_2_L";
		case 52: return "finger_ring_meta_L";
		case 53: return "finger_ring_0_L";
		case 54: return "finger_ring_1_L";
		case 55: return "finger_ring_2_L";
		case 56: return "arm_lower_L_twist";
		case 57: return "arm_lower_L_twist1";
		case 58: return "arm_upper_L_twist1";
		case 59: return "arm_upper_L_twist";
		case 62: return "finger_mid_meta_R";
		case 63: return "finger_mid_0_R";
		case 64: return "finger_mid_1_R";
		case 65: return "finger_mid_2_R";
		case 66: return "finger_pinky_meta_R";
		case 67: return "finger_pinky_0_R";
		case 68: return "finger_pinky_1_R";
		case 69: return "finger_pinky_2_R";
		case 70: return "finger_index_meta_R";
		case 71: return "finger_index_0_R";
		case 72: return "finger_index_1_R";
		case 73: return "finger_index_2_R";
		case 74: return "finger_thumb_0_R";
		case 75: return "finger_thumb_1_R";
		case 76: return "finger_thumb_2_R";
		case 77: return "finger_ring_meta_R";
		case 78: return "finger_ring_0_R";
		case 79: return "finger_ring_1_R";
		case 80: return "finger_ring_2_R";
		case 81: return "arm_lower_R_twist";
		case 82: return "arm_lower_R_twist1";
		case 83: return "arm_upper_R_twist1";
		case 84: return "arm_upper_R_twist";
		case 87: return "pect_l_aimup";
		case 88: return "pect_r_aimup";
		case 89: return "scap_aimup";
		case 90: return "pectaim_l";
		case 91: return "pecttrans_l";
		case 92: return "pectaim_r";
		case 93: return "pecttrans_r";
		case 94: return "scap_r_aimat";
		case 95: return "scap_l_aimat";
		case 96: return "pect_l_ptbase";
		case 97: return "pect_r_ptbase";
		case 98: return "ball_L";
		case 99: return "leg_upper_L_twist";
		case 100: return "leg_upper_L_twist1";
		case 101: return "ball_R";
		case 102: return "leg_upper_R_twist";
		case 103: return "leg_upper_R_twist1";
		case 104: return "feet_L";
		case 105: return "leg_upper_L_jiggle";
		case 106: return "leg_upper_L_jiggle2";
		case 107: return "climbinggear_01";
		case 108: return "climbinggear_02";
		case 109: return "feet_R";
		case 110: return "leg_upper_R_jiggle";
		case 111: return "leg_upper_R_jiggle2";
		case 112: return "holster";
		case 113: return "pistol_attachment";
		case 114: return "knife_attachment";
		case 124: return "main_weapon_attachment";
		default: return nullptr;
		}
	}
}

int PreviousTotalHits = 0;

void Menu();
void Visual(const CEntity&);
void Radar(Base_Radar, const CEntity&);
void Trigger(const CEntity&, const int&);
void AIM(const CEntity&, std::vector<AimControl::AimPoint>&);
void MiscFuncs(CEntity&);
void RenderCrosshair(ImDrawList*, const CEntity&);
void RadarSetting(Base_Radar&);

void Cheats::Run()
{	
	Menu();

	Misc::AutoAccept::UpdateAutoAccept();

#ifndef DBDEBUG
	if (!Init::Client::isGameWindowActive() && !MenuConfig::ShowMenu) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		return;
	}
#endif

	// Update matrix (do not hard-fail the whole frame)
	// If matrix read temporarily fails, we still want spectator list/radar/etc.
	(void)memoryManager.ReadMemory(gGame.GetMatrixAddress(), gGame.View.Matrix, 64);

	// Update GlobalVars every frame
	g_globalVars->UpdateGlobalvars();

	// Update EntityList Entry
	gGame.UpdateEntityListEntry();

	DWORD64 LocalControllerAddress = 0;
	DWORD64 LocalPawnAddress = 0;

	// Don't hard-fail the overlay if local pointers can't be read.
	memoryManager.ReadMemory(gGame.GetLocalControllerAddress(), LocalControllerAddress);
	memoryManager.ReadMemory(gGame.GetLocalPawnAddress(), LocalPawnAddress);
	if (LocalPawnAddress == 0 || LocalControllerAddress == 0)
		cachedResults.clear();

	// LocalEntity
	CEntity LocalEntity;
	int LocalPlayerControllerIndex = 0;
	LocalEntity.UpdateClientData();
	const bool localControllerOk = LocalEntity.UpdateController(LocalControllerAddress);
	const bool localPawnOk = LocalEntity.UpdatePawn(LocalPawnAddress);

	// If we can't resolve core pawn data, we still keep running to draw menu overlays.
	// (Watermark and spectator list need only to reach MiscFuncs.)
	if (!localControllerOk && LocalEntity.Pawn.Address == 0)
		cachedResults.clear();
	bool canProcessEntities = localPawnOk && LocalEntity.Pawn.Address != 0;
	bool canTrigger = LegitBotConfig::TriggerBot && LocalEntity.Pawn.Address != 0 && LocalEntity.Controller.Address != 0;

	// Update m_currentTick
	// Trigger/AIM are gated on tick change; ensure we keep reading tick base whenever we have a valid controller pointer.
	if (LocalEntity.Controller.Address != 0)
	{
		bool success = memoryManager.ReadMemory<DWORD>(LocalEntity.Controller.Address + Offset.PlayerController.m_nTickBase, m_currentTick);
		if (!success)
			m_currentTick = 0;
	}
	else
		m_currentTick = 0;

	// aimbot data
	std::vector<AimControl::AimPoint> AimPosList;

	// radar data
	Base_Radar GameRadar;
	if (canProcessEntities && ((RadarCFG::ShowRadar && LocalEntity.Controller.TeamID != 0) || (RadarCFG::ShowRadar && MenuConfig::ShowMenu)))
		RadarSetting(GameRadar);

	if (canProcessEntities)
	{
		// process entities
		auto entityResults = ProcessEntities(LocalEntity, LocalPlayerControllerIndex);

		// render, collect aim data
		HandleEnts(entityResults, LocalEntity, LocalPlayerControllerIndex, GameRadar, AimPosList);
	}

	if (canProcessEntities)
	{
		Visual(LocalEntity);
		Radar(GameRadar, LocalEntity);
	}
	MiscFuncs(LocalEntity);

	// Run RCS recorder continuously during calibration (not tick-gated).
	if (canProcessEntities && LegitBotConfig::RCS && (RCS::CalibrationRecording || RCS::PendingStartCalibration || RCS::PendingStopAndSave || RCS::PendingDiscard))
		RCS::RecoilControl(LocalEntity);

	int currentFPS = static_cast<int>(ImGui::GetIO().Framerate);
	if (currentFPS > MenuConfig::RenderFPS)
	{
		int FrameWait = round(1000.0f / MenuConfig::RenderFPS);
		std::this_thread::sleep_for(std::chrono::milliseconds(FrameWait));
	}

	// Run trigger every frame when the hotkey (or TriggerAlways) is active.
	// Aim remains tick-gated.
	if (canTrigger)
		Trigger(LocalEntity, LocalPlayerControllerIndex);

	// run aim / tick-based logic
	const bool canAimNow = canProcessEntities && !AimPosList.empty();
	const bool tickChanged = (m_currentTick != m_previousTick);
	const bool tickStuck = (m_currentTick == 0 && m_previousTick == 0);
	static ULONGLONG lastSpecUpdate = 0;
	const ULONGLONG nowTickMs = GetTickCount64();

	if (tickChanged || (tickStuck && canAimNow))
	{
		if (canProcessEntities)
		{
			AIM(LocalEntity, AimPosList);
		}
		

		
		// Update web radar
		if (WebRadarCFG::Enabled && WebRadar::g_webRadar && WebRadar::g_webRadar->IsEnabled())
		{
			// Collect bomb data - check if planted or held by a player
			WebRadar::BombData bombData{};
			bool bombFound = false;
			
			// Check if bomb is actually planted using the flag at PlantedC4 - 0x8
			auto plantedAddress = gGame.GetClientDLLAddress() + Offset.PlantedC4;
			bool isBombPlanted = false;
			memoryManager.ReadMemory<bool>(plantedAddress - 0x8, isBombPlanted);
			
			if (isBombPlanted)
			{
				uintptr_t plantedC4 = 0;
				if (memoryManager.ReadMemory<uintptr_t>(plantedAddress, plantedC4) && plantedC4)
				{
					uintptr_t c4Entity = 0;
					if (memoryManager.ReadMemory<uintptr_t>(plantedC4, c4Entity) && c4Entity)
					{
						// Read position via: C4 entity -> m_pGameSceneNode -> m_vecAbsOrigin
						DWORD64 gameSceneNode = 0;
						if (memoryManager.ReadMemory<DWORD64>(c4Entity + Offset.Pawn.GameSceneNode, gameSceneNode) && gameSceneNode)
						{
							Vec3 bombPos{};
							if (memoryManager.ReadMemory<Vec3>(gameSceneNode + 0xD0, bombPos))
							{
								bombData.position = bombPos;
								bombData.isPlanted = true;
								memoryManager.ReadMemory<int>(c4Entity + Offset.C4.m_nBombSite, bombData.bombSite);
								memoryManager.ReadMemory<bool>(c4Entity + Offset.C4.m_bBeingDefused, bombData.isBeingDefused);
								bombFound = true;
							}
						}
					}
				}
			}
			
			// If bomb not planted, check if any player has it
			if (!bombFound)
			{
				// Check if any player is carrying the bomb
				auto checkEntityForBomb = [&](const CEntity& entity) -> bool
				{
					if (!entity.IsAlive() || entity.Pawn.Address == 0)
						return false;
					
					// Quick check: is the active weapon the bomb?
					if (entity.Pawn.WeaponName == "c4")
					{
						bombData.position = entity.Pawn.Pos;
						bombData.isPlanted = false;
						bombData.bombSite = -1;
						bombData.isBeingDefused = false;
						bombFound = true;
						return true;
					}
					
					// Full check: scan weapon inventory for C4 (weapon ID 49)
					auto inventory = entity.Pawn.GetWeaponInventory(gGame.GetEntityListAddress());
					for (short weaponID : inventory)
					{
						if (weaponID == 49)
						{
							bombData.position = entity.Pawn.Pos;
							bombData.isPlanted = false;
							bombData.bombSite = -1;
							bombData.isBeingDefused = false;
							bombFound = true;
							return true;
						}
					}
					return false;
				};
				
				// Check local player first
				if (!checkEntityForBomb(LocalEntity))
				{
					// Check all other players
					for (const auto& [entityIndex, entity] : cachedResults)
					{
						if (checkEntityForBomb(entity))
							break;
					}
				}
			}
			
			// If bomb not planted and no player has it, scan entity list for dropped C4
			if (!bombFound)
			{
				// The weapon definition index offset chain (flat sum, no pointer deref needed)
				DWORD weaponDefIndexOffset = Offset.EconEntity.AttributeManager + 
					Offset.WeaponBaseData.Item + Offset.WeaponBaseData.ItemDefinitionIndex;
				
				// Scan entity list beyond player slots (64+) for weapon entities
				// Entity list is a two-level table: base -> chunk -> entity
				DWORD64 entityListBase = 0;
				if (memoryManager.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), entityListBase) && entityListBase)
				{
					// Scan chunks 0-1 (entities 0-1023), which covers most weapon entities
					for (int chunkIdx = 0; chunkIdx < 2 && !bombFound; chunkIdx++)
					{
						DWORD64 chunkPtr = 0;
						if (!memoryManager.ReadMemory<DWORD64>(entityListBase + 0x10 + chunkIdx * 8, chunkPtr) || !chunkPtr)
							continue;
						
						// Start from slot 65 in chunk 0 (skip player controllers), scan all of chunk 1
						int startSlot = (chunkIdx == 0) ? 65 : 0;
						int endSlot = 512;
						
						for (int slot = startSlot; slot < endSlot && !bombFound; slot++)
						{
							DWORD64 entityPtr = 0;
							if (!memoryManager.ReadMemory<DWORD64>(chunkPtr + slot * 0x70, entityPtr) || !entityPtr)
								continue;
							
							// Check if this entity is a C4 weapon (definition index 49)
							short weaponID = 0;
							if (!memoryManager.ReadMemory<short>(entityPtr + weaponDefIndexOffset, weaponID))
								continue;
							
							if (weaponID == 49)
							{
								// Found dropped C4! Read its position via GameSceneNode -> m_vecAbsOrigin
								DWORD64 gameSceneNode = 0;
								if (memoryManager.ReadMemory<DWORD64>(entityPtr + Offset.Pawn.GameSceneNode, gameSceneNode) && gameSceneNode)
								{
									Vec3 bombPos{};
									if (memoryManager.ReadMemory<Vec3>(gameSceneNode + 0xD0, bombPos))
									{
										bombData.position = bombPos;
										bombData.isPlanted = false;
										bombData.bombSite = -1;
										bombData.isBeingDefused = false;
										bombFound = true;
									}
								}
							}
						}
					}
				}
			}
			
			WebRadar::g_webRadar->UpdateRadarData(cachedResults, LocalEntity, LocalPlayerControllerIndex, m_currentTick, 
				bombFound ? &bombData : nullptr);
		}
		
		m_previousTick = m_currentTick;
	}

	// Spectator list: update at a time interval (not server-tick gated).
	if (canProcessEntities && MiscCFG::SpecList && (nowTickMs - lastSpecUpdate >= 250))
	{
		std::vector<CEntity> allEntities;
		allEntities.reserve(cachedResults.size());
		for (const auto& pair : cachedResults) {
			allEntities.push_back(pair.second);
		}
		SpecList::GetSpectatorList(allEntities, LocalEntity);
		lastSpecUpdate = nowTickMs;
	}
}

// collect entity data
std::vector<std::pair<int, CEntity>> Cheats::CollectEntityData(CEntity& localEntity, int& localPlayerControllerIndex)
{
	// update only on new tick
	//if (m_currentTick == m_previousTick)
	//{
	//	return cachedResults;
	//}

	std::vector<EntityBatchData> batchData;
	batchData.reserve(64);

	// collect entity addresses
	for (int entityIndex = 0; entityIndex < 64; ++entityIndex)
	{
		DWORD64 entityAddress = 0;
		if (!memoryManager.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + (entityIndex + 1) * 0x70, entityAddress))
		{
			continue;
		}

		// skip local player
		if (entityAddress == localEntity.Controller.Address)
		{
			localPlayerControllerIndex = entityIndex;
			continue;
		}

		// get pawn address
		CEntity tempEntity;
		tempEntity.Controller.Address = entityAddress;
		DWORD64 pawnAddress = tempEntity.Controller.GetPlayerPawnAddress();
		
		if (pawnAddress != 0)
		{
			batchData.emplace_back(entityIndex, entityAddress, pawnAddress);
		}
	}

	if (batchData.empty())
	{
		return {};
	}

	// process all entities in batch
	std::vector<std::pair<int, CEntity>> entities;
	EntityBatchProcessor processor;
	if (!processor.ProcessAllEntities(entities, batchData))
	{
		return {};
	}

	// update cache
	cachedResults = entities;

	return cachedResults;
}

// process, prepare results
std::vector<EntityResult> Cheats::ProcessEntities(CEntity& localEntity, int& localPlayerControllerIndex)
{
	// get batch-processed entities
	auto entities = CollectEntityData(localEntity, localPlayerControllerIndex);
	std::vector<EntityResult> results;
	results.reserve(entities.size());

	// process each entity
	for (auto& [entityIndex, entity] : entities)
	{
		EntityResult result;
		result.entityIndex = entityIndex;
		result.entity = entity;

		if (!entity.IsAlive())
			continue;

		// skip teammates if team check enabled
		if (MenuConfig::TeamCheck && entity.Pawn.TeamID == localEntity.Pawn.TeamID)
			continue;

		// check if in screen
		result.isInScreen = entity.IsInScreen();

		// calculate distance
		result.distance = static_cast<int>(entity.Pawn.Pos.DistanceTo(localEntity.Pawn.Pos) / 100);

		// calculate esp box rect
		if (ESPConfig::ESPenabled && result.isInScreen)
			result.espRect = ESP::GetBoxRect(entity, ESPConfig::BoxType);

		// sound esp
		if (ESPConfig::ESPenabled && ESPConfig::EnemySound && result.entity.Controller.Address != localEntity.Controller.Address)
			SoundESP::ProcessSound(result.entity, localEntity);

		result.isValid = true;
		results.push_back(result);
	}
	
	return results;
}

// render, collect aim data
void Cheats::HandleEnts(const std::vector<EntityResult>& entities, CEntity& localEntity,
    int localPlayerControllerIndex, Base_Radar& gameRadar, std::vector<AimControl::AimPoint>& aimPosList)
{
	// healthbar map (static)
	static std::map<DWORD64, Render::HealthBar> HealthBarMap;

	// aimbot data
	float MaxAimDistance = 100000;
	AimControl::ClearDebugSamples();

	const auto indexToMask = [](int idx) -> DWORD64 {
		if (idx < 0 || idx >= 64)
			return 0ull;
		return DWORD64(1) << idx;
	};
	const DWORD64 localMask = indexToMask(localPlayerControllerIndex);

	constexpr float DEG_TO_RAD = M_PI / 180.f;
	const Vec2 screenCenter{ Gui.Window.Size.x / 2.f, Gui.Window.Size.y / 2.f };
	float referenceFov = static_cast<float>(localEntity.Pawn.Fov);
	if (!std::isfinite(referenceFov) || referenceFov < 1.f || referenceFov > 179.f)
		referenceFov = 90.f;
	const float referenceTan = tanf(referenceFov * DEG_TO_RAD / 2.f);
	const float radiusScale = std::min(Gui.Window.Size.x, Gui.Window.Size.y) / 2.f;
	const bool enforceAimFov = AimControl::AimFov > 0.01f && referenceTan > 0.f;
	const float aimFovDeg = std::clamp(AimControl::AimFov, 0.1f, 179.f);
	float aimFovRadius = std::numeric_limits<float>::infinity();
	float aimFovRadiusSq = aimFovRadius * aimFovRadius;
	if (enforceAimFov)
	{
		float aimTan = tanf(aimFovDeg * DEG_TO_RAD / 2.f);
		aimFovRadius = (aimTan / referenceTan) * radiusScale;
		aimFovRadiusSq = aimFovRadius * aimFovRadius;
	}
	float minAimFovRadiusSq = 0.f;
	if (enforceAimFov && AimControl::AimFovMin > 0.01f)
	{
		float minFovDeg = std::clamp(AimControl::AimFovMin, 0.1f, aimFovDeg);
		float minTan = tanf(minFovDeg * DEG_TO_RAD / 2.f);
		float minRadius = (minTan / referenceTan) * radiusScale;
		minAimFovRadiusSq = minRadius * minRadius;
	}

	for (const auto& result : entities)
	{
		if (!result.isValid)
		{
			if (HealthBarMap.count(result.entity.Controller.Address))
				HealthBarMap.erase(result.entity.Controller.Address);
			continue;
		}

		const auto& entity = result.entity;
		const int entityIndex = result.entityIndex;

		const DWORD64 entityMask = indexToMask(entityIndex);
		const bool targetMaskVisible = ((entity.Pawn.bSpottedByMask & localMask) != 0);

		// add entity to radar
		if (RadarCFG::ShowRadar && localEntity.Controller.TeamID != 0)
		{
			gameRadar.AddPoint(localEntity.Pawn.Pos, localEntity.Pawn.ViewAngle.y, 
				entity.Pawn.Pos, ImColor(237, 85, 106, 200), RadarCFG::RadarType, entity.Pawn.ViewAngle.y);
		}

		// Out-of-FOV arrow
		if (localEntity.IsAlive()) {
			ESP::RenderOutOfFOVArrow(localEntity, result.entity);
		}

			// process aimbot data: respect configured hitbox ordering and FOV
			if (!AimControl::HitboxList.empty()) {
				const auto& bonePosList = entity.GetBone().BonePosList;
				if (bonePosList.empty())
					continue;
				Vec3 bboxMin{ FLT_MAX, FLT_MAX, FLT_MAX };
				Vec3 bboxMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
				bool hasBbox = false;
				for (const auto& bp : bonePosList) {
					const Vec3& bpPos = bp.Pos;
					if (!std::isfinite(bpPos.x) || !std::isfinite(bpPos.y) || !std::isfinite(bpPos.z))
						continue;
					hasBbox = true;
					bboxMin.x = std::min(bboxMin.x, bpPos.x);
					bboxMin.y = std::min(bboxMin.y, bpPos.y);
					bboxMin.z = std::min(bboxMin.z, bpPos.z);
					bboxMax.x = std::max(bboxMax.x, bpPos.x);
					bboxMax.y = std::max(bboxMax.y, bpPos.y);
					bboxMax.z = std::max(bboxMax.z, bpPos.z);
				}
				Vec3 bboxCorners[8]{};
				if (hasBbox) {
					bboxCorners[0] = { bboxMin.x, bboxMin.y, bboxMin.z };
					bboxCorners[1] = { bboxMin.x, bboxMin.y, bboxMax.z };
					bboxCorners[2] = { bboxMin.x, bboxMax.y, bboxMin.z };
					bboxCorners[3] = { bboxMin.x, bboxMax.y, bboxMax.z };
					bboxCorners[4] = { bboxMax.x, bboxMin.y, bboxMin.z };
					bboxCorners[5] = { bboxMax.x, bboxMin.y, bboxMax.z };
					bboxCorners[6] = { bboxMax.x, bboxMax.y, bboxMin.z };
					bboxCorners[7] = { bboxMax.x, bboxMax.y, bboxMax.z };
				}

				auto boneDamageScore = [](int boneId) -> int {
					switch (boneId)
					{
					case BONEINDEX::head: return 100;
					case BONEINDEX::neck_0: return 95;
					case BONEINDEX::spine_3: return 85;
					case BONEINDEX::spine_2: return 80;
					case BONEINDEX::spine_1: return 78;
					case BONEINDEX::spine_0: return 76;
					case BONEINDEX::pelvis: return 82;
					default: return 70;
					}
				};
				auto tryPushCandidate = [&](const Vec3& candidate, AimControl::AimSampleKind kind, int damageScore, int boneIndex = -1, bool boneVisible = false) -> bool {
					if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z))
						return false;
					Vec2 projected;
					if (!gGame.View.WorldToScreen(candidate, projected))
						return false;
					const float dx = projected.x - screenCenter.x;
					const float dy = projected.y - screenCenter.y;
					const float distSq = dx * dx + dy * dy;
					const bool insideFov = distSq <= aimFovRadiusSq;
					const bool onScreen = (projected.x >= 0.f && projected.x <= Gui.Window.Size.x && projected.y >= 0.f && projected.y <= Gui.Window.Size.y);
					bool visibilityOk = true;
					if (LegitBotConfig::VisibleCheck)
					{
						visibilityOk = targetMaskVisible && onScreen;
					}
					if (!visibilityOk)
					{
						AimControl::AddDebugSample(candidate, projected, kind, insideFov, false, false, boneIndex);
						return false;
					}
					const bool withinFov = (!enforceAimFov || insideFov);
					AimControl::AddDebugSample(candidate, projected, kind, insideFov, true, withinFov, boneIndex);
					AimControl::AimPoint point;
					point.WorldPos = candidate;
					point.DamageScore = damageScore;
					point.Kind = kind;
					point.BoneIndex = boneIndex;
					point.ScreenPos = projected;
					point.HasScreenPos = true;
					point.ScreenDistSq = distSq;
					if (std::isfinite(aimFovRadiusSq) && aimFovRadiusSq > 0.f)
						point.ScreenDistRatio = std::clamp(distSq / aimFovRadiusSq, 0.f, 1.f);
					else
						point.ScreenDistRatio = 0.f;
					point.InsideScreenFov = withinFov;
					point.InsideScreenDeadzone = (minAimFovRadiusSq > 0.f && distSq < minAimFovRadiusSq);
					aimPosList.push_back(point);
					return true;
				};
				auto addHeadCircleSamples = [&](const Vec3& headPos, const Vec3& neckPos, int baseDamage, bool& headAccepted, bool baseVisible) {
					auto isFiniteVec = [](const Vec3& v) {
						return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
					};
					if (!isFiniteVec(headPos) || !isFiniteVec(neckPos))
						return;
					Vec3 up = headPos - neckPos;
					const float upLenSq = LengthSquared(up);
					if (upLenSq < 1e-4f)
						return;
					const float upLen = sqrtf(upLenSq);
					up = up / upLen;
					Vec3 tangent = CrossProduct(up, Vec3{ 0.f, 0.f, 1.f });
					if (LengthSquared(tangent) < 1e-4f)
						tangent = CrossProduct(up, Vec3{ 0.f, 1.f, 0.f });
					if (LengthSquared(tangent) < 1e-4f)
						tangent = CrossProduct(up, Vec3{ 1.f, 0.f, 0.f });
					if (LengthSquared(tangent) < 1e-4f)
						return;
					tangent.Normalize();
					Vec3 bitangent = CrossProduct(up, tangent);
					if (LengthSquared(bitangent) < 1e-4f)
						return;
					bitangent.Normalize();
					const float radius = std::clamp(upLen * 0.65f, 1.2f, 8.5f);
					constexpr int circleSegments = 6;
					const int circleDamage = std::max(baseDamage - 5, 60);
					for (int i = 0; i < circleSegments; ++i)
					{
						if (i == 0 || i == circleSegments / 2)
							continue;
						float angle = (2.0f * 3.14159265f * i) / circleSegments;
						float c = cosf(angle);
						float s = sinf(angle);
						Vec3 offset = (tangent * c + bitangent * s) * radius;
						if (tryPushCandidate(headPos + offset, AimControl::AimSampleKind::Bone, circleDamage, BONEINDEX::head, baseVisible))
							headAccepted = true;
					}
					const float crownLift = radius * 0.6f;
					const float crownRadius = radius * 0.5f;
					if (crownLift > 0.05f)
					{
						Vec3 apex = headPos + up * crownLift;
						if (tryPushCandidate(apex, AimControl::AimSampleKind::Bone, circleDamage, BONEINDEX::head, baseVisible))
							headAccepted = true;
						for (int i = 0; i < circleSegments; ++i)
						{
							if (i == 0 || i == circleSegments / 2)
								continue;
							float angle = (2.0f * 3.14159265f * i) / circleSegments;
							float c = cosf(angle);
							float s = sinf(angle);
							Vec3 offset = up * crownLift + (tangent * c + bitangent * s) * crownRadius;
							if (tryPushCandidate(headPos + offset, AimControl::AimSampleKind::Bone, circleDamage, BONEINDEX::head, baseVisible))
								headAccepted = true;
						}
					}
				};

				bool hasPrimary = false;
				bool headCircleAttempted = false;
				for (int hb : AimControl::HitboxList) {
					if (hb < 0 || static_cast<size_t>(hb) >= bonePosList.size())
						continue;
					Vec3 tempPos = bonePosList[hb].Pos;
					if (hb == BONEINDEX::head && (AimControl::HeadOffset != 0.f || AimControl::HeadDropOffset != 0.f))
					{
						if (AimControl::HeadOffset != 0.f)
						{
							float yawRad = entity.Pawn.ViewAngle.y * (3.14159265f / 180.f);
							Vec3 backDir{ -cosf(yawRad), -sinf(yawRad), 0.f };
							float backLen = sqrtf(backDir.x * backDir.x + backDir.y * backDir.y);
							if (backLen > 0.01f)
								tempPos = tempPos + (backDir / backLen) * AimControl::HeadOffset;
						}
						if (AimControl::HeadDropOffset != 0.f)
						{
							const size_t neckIdx = static_cast<size_t>(BONEINDEX::neck_0);
							if (neckIdx < bonePosList.size())
							{
								Vec3 dir = bonePosList[neckIdx].Pos - tempPos;
								float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
								if (dirLen > 0.01f)
									tempPos = tempPos + (dir / dirLen) * AimControl::HeadDropOffset;
							}
						}
					}
					if (tryPushCandidate(tempPos, AimControl::AimSampleKind::Bone, boneDamageScore(hb), hb, bonePosList[hb].IsVisible)) {
						hasPrimary = true;
					}
					if (hb == BONEINDEX::head && !headCircleAttempted)
					{
						headCircleAttempted = true;
						const size_t neckIdx = static_cast<size_t>(BONEINDEX::neck_0);
						if (neckIdx < bonePosList.size())
						{
							addHeadCircleSamples(tempPos, bonePosList[neckIdx].Pos, boneDamageScore(BONEINDEX::head), hasPrimary, bonePosList[hb].IsVisible);
					}
				}
				}

				Vec3 bboxCenter{
					(bboxMin.x + bboxMax.x) * 0.5f,
					(bboxMin.y + bboxMax.y) * 0.5f,
					(bboxMin.z + bboxMax.z) * 0.5f
				};
				const float height = bboxMax.z - bboxMin.z;
				if (!hasPrimary && hasBbox && AimControl::UseEdgeSampling)
				{
					static constexpr std::array<float, 3> edgeFractions{ 0.25f, 0.5f, 0.75f };
					for (float fraction : edgeFractions)
					{
						float z = bboxMin.z + height * fraction;
						Vec3 candidates[4]{
							{ bboxMin.x, bboxCenter.y, z },
							{ bboxMax.x, bboxCenter.y, z },
							{ bboxCenter.x, bboxMin.y, z },
							{ bboxCenter.x, bboxMax.y, z }
						};
						for (const auto& edgePoint : candidates)
						{
							tryPushCandidate(edgePoint, AimControl::AimSampleKind::Edge, 65);
						}
					}
				}

				if (!hasPrimary && hasBbox) {
					static constexpr std::array<float, 3> bodySamples{ 0.7f, 0.5f, 0.35f };
					for (float fraction : bodySamples) {
						Vec3 sample = bboxCenter;
						sample.z = bboxMin.z + height * fraction;
						if (tryPushCandidate(sample, AimControl::AimSampleKind::Body, 70)) {
							break;
						}
					}
				}

				if (!hasPrimary && hasBbox) {
					for (const auto& corner : bboxCorners) {
						if (tryPushCandidate(corner, AimControl::AimSampleKind::Corner, 55)) {
							break;
						}
					}
				}

				if (hasBbox && ESPConfig::ShowHitboxBBox) {
                    static const int edges[12][2] = {
                        {0,1},{0,2},{0,4},{1,3},{1,5},{2,3},
                        {2,6},{3,7},{4,5},{4,6},{5,7},{6,7}
                    };
                    ImVec2 projected[8];
                    bool cornerVisible[8] = {};
                    for (int i = 0; i < 8; ++i) {
                        Vec2 screen;
                        if (gGame.View.WorldToScreen(bboxCorners[i], screen)) {
                            projected[i] = ImVec2(screen.x, screen.y);
                            cornerVisible[i] = true;
                        }
                    }
                    auto drawList = ImGui::GetBackgroundDrawList();
                    ImU32 bboxColor = ESPConfig::BoneColor;
                    for (const auto& edge : edges) {
                        if (cornerVisible[edge[0]] && cornerVisible[edge[1]]) {
                            drawList->AddLine(projected[edge[0]], projected[edge[1]], bboxColor, 1.0f);
                        }
                    }

                    static const std::pair<int, float> perimeterBones[] = {
                        {BONEINDEX::head, 12.f},
                        {BONEINDEX::neck_0, 9.f},
                        {BONEINDEX::spine_3, 14.f},
                        {BONEINDEX::spine_2, 18.f},
                        {BONEINDEX::spine_1, 20.f},
                        {BONEINDEX::pelvis, 18.f},
                        {BONEINDEX::arm_upper_L, 10.f},
                        {BONEINDEX::arm_lower_L, 8.f},
                        {BONEINDEX::hand_L, 6.f},
                        {BONEINDEX::arm_upper_R, 10.f},
                        {BONEINDEX::arm_lower_R, 8.f},
                        {BONEINDEX::hand_R, 6.f},
                        {BONEINDEX::leg_upper_L, 10.f},
                        {BONEINDEX::leg_lower_L, 8.f},
                        {BONEINDEX::ankle_L, 7.f},
                        {BONEINDEX::leg_upper_R, 10.f},
                        {BONEINDEX::leg_lower_R, 8.f},
                        {BONEINDEX::ankle_R, 7.f}
                    };
                    float screenScale = Gui.Window.Size.y / 1080.f;
                    for (const auto& [boneId, radius] : perimeterBones) {
                        if (boneId < 0 || static_cast<size_t>(boneId) >= bonePosList.size())
                            continue;
                        const auto& bp = bonePosList[boneId];
                        if (!bp.IsVisible)
                            continue;
                        ImVec2 pos{ bp.ScreenPos.x, bp.ScreenPos.y };
                        drawList->AddCircle(pos, radius * screenScale, bboxColor, 32, 1.2f);
                    }
                }
            }

		// render esp
		if (!result.isInScreen)
			continue;

		if (ESPConfig::ESPenabled && (!ESPConfig::FlashCheck || localEntity.Pawn.FlashDuration < 0.1f))
		{
			const ImVec4& Rect = result.espRect;
			const int distance = result.distance;

			if (MenuConfig::RenderDistance == 0 || (distance <= MenuConfig::RenderDistance && MenuConfig::RenderDistance > 0))
			{
				ESP::RenderPlayerESP(localEntity, entity, Rect, localPlayerControllerIndex, entityIndex);
				Render::DrawDistance(localEntity, entity, Rect);

				// healthbar
				if(ESPConfig::ShowHealthBar || ESPConfig::ShowHealthNum)
				{
					ImVec2 HealthBarPos = { Rect.x - 6.f, Rect.y };
					ImVec2 HealthBarSize = { 4, Rect.w };
					Render::DrawHealthBar(entity.Controller.Address, 100, entity.Pawn.Health, HealthBarPos, HealthBarSize);
				}


				// ammo
				// When player is using knife or nade, Ammo = -1.
				if (ESPConfig::AmmoBar && entity.Pawn.Ammo != -1)
				{
					ImVec2 AmmoBarPos = { Rect.x, Rect.y + Rect.w + 2 };
					ImVec2 AmmoBarSize = { Rect.z, 4 };
					Render::DrawAmmoBar(entity.Controller.Address, entity.Pawn.Ammo + entity.Pawn.ShotsFired, 
						entity.Pawn.Ammo, AmmoBarPos, AmmoBarSize);
				}

                // armor
                // It is meaningless to render an empty bar
                if ((ESPConfig::ArmorBar || ESPConfig::ShowArmorNum) && entity.Pawn.Armor > 0)
                {
                    bool HasHelmet;
                    ImVec2 ArmorBarPos;
                    memoryManager.ReadMemory(entity.Controller.Address + Offset.PlayerController.HasHelmet, HasHelmet);
                    if (ESPConfig::ShowHealthBar)
                        ArmorBarPos = { Rect.x - 10.f, Rect.y };
                    else
                        ArmorBarPos = { Rect.x - 6.f, Rect.y };
                    ImVec2 ArmorBarSize = { 4.f, Rect.w };
                    Render::DrawArmorBar(entity.Controller.Address, 100, entity.Pawn.Armor, HasHelmet, ArmorBarPos, ArmorBarSize);
                }

				// display visible bone names/IDs
				if (ESPConfig::ShowBoneESP && ESPConfig::ShowBoneLabels)
				{
					auto drawList = ImGui::GetBackgroundDrawList();
					const auto& boneList = entity.GetBone().BonePosList;
					ImU32 visibleColor = ESPConfig::BoneColor;
					const ImU32 occludedColor = IM_COL32(160, 160, 160, 220);
					for (size_t bi = 0; bi < boneList.size(); ++bi)
					{
						const auto& bp = boneList[bi];
						Vec2 projected = bp.ScreenPos;
						if (!bp.IsVisible)
						{
							if (!gGame.View.WorldToScreen(bp.Pos, projected))
								continue;
						}
						ImVec2 pos{ projected.x, projected.y };
						const char* friendly = BoneFriendlyName(bi);
						char buf[64];
						if (friendly)
							std::snprintf(buf, sizeof(buf), "[%03zu] %s", bi, friendly);
						else
							std::snprintf(buf, sizeof(buf), "[%03zu] bone_%03zu", bi, bi);
						const ImU32 labelColor = bp.IsVisible ? visibleColor : occludedColor;
						drawList->AddText(pos, labelColor, buf);
					}
				}

				{
					static std::unordered_map<DWORD64, ULONGLONG> s_lastBoneDump;
					const auto& boneList = entity.GetBone().BonePosList;
					if (ESPConfig::DumpBoneData && !boneList.empty())
					{
						ULONGLONG now = GetTickCount64();
						ULONGLONG& lastTick = s_lastBoneDump[entity.Controller.Address];
						if (now - lastTick >= 2000)
						{
							lastTick = now;
							std::printf("[BoneDump] entity=0x%llx bones=%zu\n", entity.Controller.Address, boneList.size());
							for (size_t bi = 0; bi < boneList.size(); ++bi)
							{
								const auto& bp = boneList[bi];
								const char* friendly = BoneFriendlyName(bi);
								std::printf("  [%03zu]%s vis=%d pos=(%.2f, %.2f, %.2f)\n", bi,
									friendly ? friendly : "",
									bp.IsVisible ? 1 : 0,
									bp.Pos.x, bp.Pos.y, bp.Pos.z);
							}
						}
					}
					else if (!ESPConfig::DumpBoneData && !s_lastBoneDump.empty())
					{
						s_lastBoneDump.clear();
					}
				}
			}
		}
	}
}

void Menu() 
{
	if (MenuConfig::ShowMenu)
		GUI::DrawGui();

	GUI::InitHitboxList();
}

void Visual(const CEntity& LocalEntity)
{
	// Fov circle
	if (LocalEntity.Pawn.TeamID != 0 && !MenuConfig::ShowMenu)
		Render::DrawFovCircle(ImGui::GetBackgroundDrawList(), LocalEntity);

	// Fov line
	Render::DrawFov(LocalEntity, LegitBotConfig::FovLineSize, LegitBotConfig::FovLineColor, 1);

	// HeadShoot Line
	Render::HeadShootLine(LocalEntity, MiscCFG::HeadShootLineColor);

    RenderCrosshair(ImGui::GetBackgroundDrawList(), LocalEntity);

    // visualize current aim target and direction
    auto drawList = ImGui::GetBackgroundDrawList();
	if (AimControl::HasTarget) {
		ImVec2 target{ AimControl::LastTargetScreenPos.x, AimControl::LastTargetScreenPos.y };
		ImVec2 center{ Gui.Window.Size.x / 2, Gui.Window.Size.y / 2 };
		drawList->AddLine(center, target, ImColor(237, 85, 106, 200), 1.5f);
		// overlay current targeted bone at top
	}

	if (ESPConfig::ShowAimSamples && !AimControl::DebugSamples.empty())
	{
		auto sampleLabel = [](AimControl::AimSampleKind kind, int boneIndex) -> std::string
		{
			switch (kind)
			{
			case AimControl::AimSampleKind::Bone:
			{
				const char* name = BoneFriendlyName(boneIndex);
				if (name)
					return std::string("B:") + name;
				else
					return "B:" + std::to_string(boneIndex);
			}
			case AimControl::AimSampleKind::Body: return "Body";
			case AimControl::AimSampleKind::Edge: return "Edge";
			case AimControl::AimSampleKind::Corner: return "Corner";
			default: return "?";
			}
		};
		const bool drawLabels = AimControl::DebugSamples.size() <= 80;
		for (const auto& sample : AimControl::DebugSamples)
		{
			ImColor color;
			if (!sample.VisibilityOk)
				color = ImColor(130, 130, 130, 200);
			else if (sample.Accepted)
				color = ImColor(72, 201, 127, 235);
			else if (sample.InsideFov)
				color = ImColor(255, 196, 0, 220);
			else
				color = ImColor(226, 99, 99, 220);
			float radius = sample.Accepted ? 5.0f : 3.0f;
			ImVec2 screen{ sample.ScreenPos.x, sample.ScreenPos.y };
			drawList->AddCircleFilled(screen, radius, color, 0);
			if (drawLabels)
			{
				drawList->AddText(ImVec2(screen.x + 5.0f, screen.y - 6.0f), IM_COL32(255, 255, 255, 220), sampleLabel(sample.Kind, sample.BoneIndex).c_str());
			}
		}
	}
}

void Radar(Base_Radar Radar, const CEntity& LocalEntity)
{
	// Radar render
	if ((RadarCFG::ShowRadar && LocalEntity.Controller.TeamID != 0) || (RadarCFG::ShowRadar && MenuConfig::ShowMenu))
	{
		Radar.Render();

		MenuConfig::RadarWinPos = ImGui::GetWindowPos();
		ImGui::End();
	}
}

void Trigger(const CEntity& LocalEntity, const int& LocalPlayerControllerIndex)
{
	// TriggerBot
	if (LegitBotConfig::TriggerBot && (GetAsyncKeyState(TriggerBot::HotKey) || LegitBotConfig::TriggerAlways))
	{
		TriggerBot::Run(LocalEntity, LocalPlayerControllerIndex);
	}
}

void AIM(const CEntity& LocalEntity, std::vector<AimControl::AimPoint>& AimPosList) {
	static ULONGLONG lastTick = 0;
	ULONGLONG currentTick = GetTickCount64();

	if (!LegitBotConfig::AimBot) {
		RCS::RecoilControl(LocalEntity);
		return;
	}
	// Calibration recorder lives in RCS::RecoilControl().
	// (Recorder tick is called from Cheats::Run() to avoid tick-gating.)

    bool keyHeld = (GetAsyncKeyState(AimControl::HotKey) & 0x8000) != 0;
    // Determine aiming mode: always-on when non-toggle, or toggle-based when enabled
    bool shouldAim;
    if (LegitBotConfig::AimToggleMode) {
        if (keyHeld && currentTick - lastTick >= 200) {
            AimControl::switchToggle();
            lastTick = currentTick;
        }
        shouldAim = LegitBotConfig::AimAlways;
    } else {
        // non-toggle mode: only aim while hotkey is held
        shouldAim = keyHeld;
    }

	// Camera position reads can drift; fall back to entity origin.
	Vec3 localAimPos = LocalEntity.Pawn.CameraPos;
	if (!std::isfinite(localAimPos.x) || !std::isfinite(localAimPos.y) || !std::isfinite(localAimPos.z) ||
		std::fabs(localAimPos.x) > 100000.0f || std::fabs(localAimPos.y) > 100000.0f || std::fabs(localAimPos.z) > 100000.0f)
	{
		localAimPos = LocalEntity.Pawn.Pos;
	}

    if (shouldAim && !AimPosList.empty())
        AimControl::AimBot(LocalEntity, localAimPos, AimPosList);
}

void MiscFuncs(CEntity& LocalEntity)
{
    SpecList::SpectatorWindowList(LocalEntity);
    bmb::RenderWindow(LocalEntity.Controller.TeamID);
    SoundESP::Render();

    Misc::HitManager(LocalEntity, PreviousTotalHits);
    Misc::BunnyHop(LocalEntity);
    Misc::Watermark(LocalEntity);
    Misc::FastStop();
    Misc::AntiAFKKickUpdate();
    if (MiscCFG::AutoKnife && !MenuConfig::ShowMenu) {
        std::vector<CEntity> enemyList;
        enemyList.reserve(Cheats::cachedResults.size());
        for (const auto& r : Cheats::cachedResults) enemyList.push_back(r.second);
        Misc::KnifeBot(LocalEntity, enemyList);
    }
    if (MiscCFG::AutoZeus && !MenuConfig::ShowMenu) {
        Misc::ZeusBot(LocalEntity);
    }
}

void RadarSetting(Base_Radar& Radar)
{
	// Radar window
	ImGui::SetNextWindowBgAlpha(RadarCFG::RadarBgAlpha);
	ImGui::Begin("Radar", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	ImGui::SetWindowSize({ RadarCFG::RadarRange * 2,RadarCFG::RadarRange * 2 });
	ImGui::SetWindowPos(MenuConfig::RadarWinPos, ImGuiCond_Once);

	if (MenuConfig::RadarWinChengePos)
	{
		ImGui::SetWindowPos("Radar", MenuConfig::RadarWinPos);
		MenuConfig::RadarWinChengePos = false;
	}

	if (!RadarCFG::customRadar)
	{
		RadarCFG::ShowRadarCrossLine = false;
		RadarCFG::Proportion = 2700.f;
		RadarCFG::RadarPointSizeProportion = 1.f;
		RadarCFG::RadarRange = 125.f;
		RadarCFG::RadarBgAlpha = 0.1f;
	}


	// Radar.SetPos({ Gui.Window.Size.x / 2,Gui.Window.Size.y / 2 });
	Radar.SetDrawList(ImGui::GetWindowDrawList());
	Radar.SetPos({ ImGui::GetWindowPos().x + RadarCFG::RadarRange, ImGui::GetWindowPos().y + RadarCFG::RadarRange });
	Radar.SetProportion(RadarCFG::Proportion);
	Radar.SetRange(RadarCFG::RadarRange);
	Radar.SetSize(RadarCFG::RadarRange * 2);
	Radar.SetCrossColor(RadarCFG::RadarCrossLineColor);

	Radar.ArcArrowSize *= RadarCFG::RadarPointSizeProportion;
	Radar.ArrowSize *= RadarCFG::RadarPointSizeProportion;
	Radar.CircleSize *= RadarCFG::RadarPointSizeProportion;

	Radar.ShowCrossLine = RadarCFG::ShowRadarCrossLine;
	Radar.Opened = true;
}

void RenderCrosshair(ImDrawList* drawList, const CEntity& LocalEntity)
{
	if (!MiscCFG::SniperCrosshair || LocalEntity.Controller.TeamID == 0 || MenuConfig::ShowMenu)
		return;

	bool isScoped;
	memoryManager.ReadMemory<bool>(LocalEntity.Pawn.Address + Offset.Pawn.isScoped, isScoped);
	std::string curWeapon = TriggerBot::GetWeapon(LocalEntity);

	if (!TriggerBot::CheckScopeWeapon(curWeapon) || isScoped)
		return;

	Render::DrawCrossHair(drawList, ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), MiscCFG::SniperCrosshairColor);
}

std::string Cheats::GetCurrentMapName() {
    if (!g_globalVars || !g_globalVars->g_cCurrentMap) {
        return "";
    }

    char currentMap[256] = { 0 };
    if (!memoryManager.ReadMemory(reinterpret_cast<DWORD64>(g_globalVars->g_cCurrentMap),
        currentMap, sizeof(currentMap) - 1)) {
        return "";
    }

    currentMap[255] = '\0';
    return std::string(currentMap);
}
