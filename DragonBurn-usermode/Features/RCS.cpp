#include "RCS.h"

#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <mutex>
#include <cstdint>

// JSON profiles are stored separately from .cfg presets.
// This lets you calibrate once per weapon and keep it across config loads.
#include "../Core/Config.h"
#include <json.hpp>

using json = nlohmann::json;

namespace {
	struct ProfileStorage {
		std::unordered_map<std::string, RCS::WeaponProfile> profiles;
		bool loaded = false;
		std::mutex mtx;

		std::string GetProfilePath() {
			return MenuConfig::path + "\\Data\\rcs_profiles.json";
		}

		void EnsureLoaded() {
			std::lock_guard<std::mutex> lock(mtx);
			if (loaded)
				return;
			loaded = true;

			const std::string path = GetProfilePath();
			if (!std::filesystem::exists(path))
				return;

			std::ifstream f(path);
			if (!f.is_open())
				return;

			json root;
			try {
				f >> root;
			} catch (...) {
				return;
			}

			if (!root.is_object())
				return;

			for (auto it = root.begin(); it != root.end(); ++it) {
				const std::string weaponName = it.key();
				const json& node = it.value();
				if (!node.is_object())
					continue;

				auto bulletsNodeIt = node.find("bullets");
				if (bulletsNodeIt == node.end() || !bulletsNodeIt->is_array())
					continue;

				RCS::WeaponProfile profile;
				profile.firstShot = 1;
				if (node.contains("firstShot") && node["firstShot"].is_number_unsigned())
					profile.firstShot = node["firstShot"].get<uint32_t>();
				for (const auto& pairNode : *bulletsNodeIt) {
					if (!pairNode.is_array() || pairNode.size() < 2)
						continue;
					profile.bullets.emplace_back(pairNode[0].get<float>(), pairNode[1].get<float>());
				}

				if (!profile.bullets.empty())
					profiles[weaponName] = std::move(profile);
			}
		}

		void Save() {
			std::lock_guard<std::mutex> lock(mtx);
			std::filesystem::create_directories(std::filesystem::path(GetProfilePath()).parent_path());
			const std::string path = GetProfilePath();

			json root = json::object();
			for (const auto& kv : profiles) {
				json node;
				json bullets = json::array();
				node["firstShot"] = kv.second.firstShot;
				for (const auto& v : kv.second.bullets) {
					bullets.push_back({ v.x, v.y });
				}
				node["bullets"] = std::move(bullets);
				root[kv.first] = std::move(node);
			}

			std::ofstream f(path);
			if (!f.is_open())
				return;
			f << root.dump(4);
		}
	};

	ProfileStorage g_profiles;

	float NormalizeYaw(float yaw) {
		while (yaw > 180.f) yaw -= 360.f;
		while (yaw < -180.f) yaw += 360.f;
		return yaw;
	}
}

namespace RCS {
	void RequestStartCalibration() {
		CalibrationMode = true;
		// Actual recording init happens from within RCS::RecoilControl (it has LocalPlayer).
		PendingStartCalibration = true;
		PendingStopAndSave = false;
		PendingDiscard = false;
		// Show the "Stop & Save" UI immediately, even if we are waiting
		// for a clean spray (ShotsFired -> 0 -> 1) to start recording.
		CalibrationRecording = true;
	}

	void RequestStartCalibrationAuto() {
		// One-click calibration flow: close GUI, record full clip, save, reopen GUI.
		CalibrationMode = false;
		CalibrationAutoRun = true;
		PendingStartCalibration = true;
		PendingStopAndSave = false;
		PendingDiscard = false;
		CalibrationRecording = true;
		// Close GUI while user sprays.
		MenuConfig::ShowMenu = false;
	}

	void RequestStopAndSave() {
		// Stop is executed from within RCS tick to ensure we use latest sampled ammo.
		PendingStopAndSave = true;
		PendingDiscard = false;
	}

	void RequestDiscard() {
		CalibrationWeapon.clear();
		PendingDiscard = true;
		PendingStopAndSave = false;
	}

	bool IsCalibrating() {
		return CalibrationRecording;
	}

	bool GetProfileForWeapon(const std::string& weaponName, WeaponProfile& outProfile) {
		g_profiles.EnsureLoaded();
		std::lock_guard<std::mutex> lock(g_profiles.mtx);
		auto it = g_profiles.profiles.find(weaponName);
		if (it == g_profiles.profiles.end())
			return false;
		outProfile = it->second;
		return true;
	}
}

// Recorder bookkeeping (not exposed to GUI).
namespace {
	static bool s_recordingActive = false;
	static bool s_waitingForSprayStart = false;
	static std::string s_recordingWeapon;
	static Vec2 s_recordingBaselineAimPunch{};
	static DWORD s_lastShotsFired = 0;
	static uint32_t s_recordingFirstShot = 1;
	static std::vector<Vec2> s_recordedAimPunch; // aimPunchAngle per shot
	static bool s_hasBaseline = false;
	static bool s_stopAfterThisTick = false;

	bool ReadLastAimPunchCache(const CEntity& local, Vec2& outPunch) {
		const auto count = static_cast<DWORD>(local.Pawn.AimPunchCache.Count);
		if (count == 0 || count > 0xFFFF)
			return false;
		if (local.Pawn.AimPunchCache.Data == 0)
			return false;
		return memoryManager.ReadMemory<Vec2>(
			local.Pawn.AimPunchCache.Data + (count - 1) * sizeof(Vec3),
			outPunch);
	}
	
	void StartRecordingForWeapon(const CEntity& local) {
		s_recordingWeapon = local.Pawn.WeaponName;
		s_recordedAimPunch.clear();
		s_lastShotsFired = local.Pawn.ShotsFired;
		s_recordingFirstShot = static_cast<uint32_t>(s_lastShotsFired + 1);
		s_waitingForSprayStart = false;
		s_hasBaseline = true;
		// Baseline isn't strictly needed for aimPunchAngle table, but keep for sanity.
		if (!ReadLastAimPunchCache(local, s_recordingBaselineAimPunch))
			s_recordingBaselineAimPunch = local.Pawn.AimPunchAngle;
		s_recordingActive = true;
	}

	void StopAndSaveRecording() {
		if (!s_hasBaseline || s_recordingWeapon.empty()) {
			s_recordingActive = false;
			s_waitingForSprayStart = false;
			s_stopAfterThisTick = false;
			RCS::CalibrationRecording = false;
			return;
		}
		if (s_recordedAimPunch.empty()) {
			s_recordingActive = false;
			s_waitingForSprayStart = false;
			s_stopAfterThisTick = false;
			RCS::CalibrationRecording = false;
			return;
		}

		g_profiles.EnsureLoaded();
		RCS::WeaponProfile profile;
		profile.firstShot = s_recordingFirstShot;
		profile.bullets = s_recordedAimPunch;
		g_profiles.profiles[s_recordingWeapon] = std::move(profile);
		g_profiles.Save();
		s_recordingActive = false;
		s_waitingForSprayStart = false;
		RCS::CalibrationRecording = false;
		if (RCS::CalibrationAutoRun) {
			// Calibration finished: reopen GUI and stop auto-run.
			RCS::CalibrationAutoRun = false;
			RCS::CalibrationMode = false;
			MenuConfig::ShowMenu = true;
		}
		s_stopAfterThisTick = false;
	}

	void DiscardRecording() {
		s_recordingActive = false;
		s_waitingForSprayStart = false;
		s_stopAfterThisTick = false;
		s_recordingWeapon.clear();
		s_recordedAimPunch.clear();
		s_hasBaseline = false;
		RCS::CalibrationRecording = false;
		if (RCS::CalibrationAutoRun) {
			RCS::CalibrationAutoRun = false;
			RCS::CalibrationMode = false;
			MenuConfig::ShowMenu = true;
		}
	}
}

void RCS::UpdateAngles(const CEntity& Local, Vec2& Angles)
{
	Angles = Vec2(0.f, 0.f);

	Vec2 aimPunch = Local.Pawn.AimPunchAngle;

	const auto count = static_cast<DWORD>(Local.Pawn.AimPunchCache.Count);
	if (count > 0 && count < 0xFFFF && Local.Pawn.AimPunchCache.Data != 0)
	{
		Vec2 punchFromCache{};
		if (memoryManager.ReadMemory<Vec2>(
			Local.Pawn.AimPunchCache.Data + (count - 1) * sizeof(Vec3),
			punchFromCache))
		{
			aimPunch = punchFromCache;
		}
	}

	Angles.x = aimPunch.x;
	Angles.y = aimPunch.y;

	if (Angles.x > 89.f) Angles.x = 89.f;
	if (Angles.x < -89.f) Angles.x = -89.f;
	while (Angles.y > 180.f) Angles.y -= 360.f;
	while (Angles.y < -180.f) Angles.y += 360.f;
}

bool RCS::GetProfileAimPunch(const CEntity& local, Vec2& outAimPunch) {
	g_profiles.EnsureLoaded();
	const std::string weaponName = local.Pawn.WeaponName;
	if (weaponName.empty())
		return false;

	auto it = g_profiles.profiles.find(weaponName);
	if (it == g_profiles.profiles.end())
		return false;

	const auto& bullets = it->second.bullets;
	const uint32_t firstShot = it->second.firstShot;
	const DWORD shots = local.Pawn.ShotsFired;
	if (shots < firstShot || shots >= static_cast<DWORD>(firstShot + bullets.size()))
		return false;

	outAimPunch = bullets[shots - firstShot];
	return true;
}

void RCS::RecoilControl(CEntity LocalPlayer)
{
	if (!LegitBotConfig::RCS)
		return;

	// Calibration requests come from GUI, but we only execute them here (where we have LocalPlayer).
	if (RCS::PendingStartCalibration) {
		RCS::PendingStartCalibration = false;
		// Reset recorder state, but DO NOT touch CalibrationAutoRun/GUI state.
		s_recordingActive = false;
		s_waitingForSprayStart = false;
		s_stopAfterThisTick = false;
		s_recordingWeapon.clear();
		s_recordedAimPunch.clear();
		s_hasBaseline = false;
		RCS::CalibrationRecording = false;
		StartRecordingForWeapon(LocalPlayer);
		CalibrationWeapon = s_recordingWeapon;
		// Ensure UI reflects that recording has started.
		RCS::CalibrationRecording = true;
	}
	if (RCS::PendingStopAndSave) {
		RCS::PendingStopAndSave = false;
		// Defer saving by one tick so Aimbot (which runs after RCS) doesn't compensate on the stop tick.
		if (s_recordingActive)
			s_stopAfterThisTick = true;
		else
			StopAndSaveRecording();
		if (!s_recordingActive)
			if (!RCS::CalibrationAutoRun)
				CalibrationWeapon.clear();
	}
	if (RCS::PendingDiscard) {
		RCS::PendingDiscard = false;
		DiscardRecording();
		CalibrationWeapon.clear();
	}

	// If we detected end-of-clip on the previous tick, save now.
	// Do it before recording/compensation logic so Aimbot sees the correct state.
	if (s_stopAfterThisTick) {
		s_stopAfterThisTick = false;
		StopAndSaveRecording();
		if (!RCS::CalibrationAutoRun)
			CalibrationWeapon.clear();
	}

	if (s_recordingActive) {
		// Passive recording: no mouse compensation while recording.
		const DWORD shots = LocalPlayer.Pawn.ShotsFired;
		if (!s_hasBaseline) {
			// Baseline should only be valid when shots start at 0.
			s_hasBaseline = (s_lastShotsFired == 0);
		}

				// Record aim punch for each new shot.
				if (shots > s_lastShotsFired) {
					while (s_lastShotsFired < shots) {
						Vec2 punchSample{};
						if (!ReadLastAimPunchCache(LocalPlayer, punchSample))
							punchSample = LocalPlayer.Pawn.AimPunchAngle;
						s_recordedAimPunch.push_back(punchSample);
						s_lastShotsFired++;
					}
				}

		// Stop when magazine ammo is empty.
		// Pawn.Ammo uses WeaponBaseData.Clip1, so it matches magazine ammo.
		if (s_lastShotsFired > 0 && LocalPlayer.Pawn.Ammo <= 0) {
			// Defer saving by one tick so Aimbot (which runs after RCS) doesn't compensate on the last shot tick.
			s_stopAfterThisTick = true;
		}

		// If ShotsFired resets (reload/interrupt), stop too.
		if (s_lastShotsFired > 0 && shots < s_lastShotsFired) {
			s_stopAfterThisTick = true;
		}

		// Fallback: if the game resets ShotsFired to 0 (reload/interrupt)
		// after we've already recorded at least one shot, stop and save.
		if (s_lastShotsFired > 0 && shots == 0) {
			s_stopAfterThisTick = true;
		}

		return;
	}

	// Waiting for ShotsFired to reset to 0 so we can start capturing from shot 1.
	if (s_waitingForSprayStart) {
		if (LocalPlayer.Pawn.ShotsFired == 0) {
			s_waitingForSprayStart = false;
			s_recordingActive = true;
			s_lastShotsFired = 0;
			s_hasBaseline = true;
		}
		// No mouse compensation while waiting/recording.
		return;
	}

	// Playback
	static Vec2 OldPunch = { 0, 0 };
	static std::string OldWeaponName;

	// Reset per weapon to avoid carry-over between different gun types.
	if (OldWeaponName != LocalPlayer.Pawn.WeaponName) {
		OldWeaponName = LocalPlayer.Pawn.WeaponName;
		OldPunch = Vec2{ 0, 0 };
	}

	if (LocalPlayer.Pawn.ShotsFired > static_cast<DWORD>(RCSBullet)) {
		Vec2 punchAngle;
		bool hasProfile = GetProfileAimPunch(LocalPlayer, punchAngle);

		Vec2 punchNow = (hasProfile ? punchAngle : LocalPlayer.Pawn.AimPunchAngle) * 2.f;
		Vec2 delta = punchNow - OldPunch;

		int MouseX = static_cast<int>(std::round((delta.y / LocalPlayer.Client.Sensitivity) / 0.011f));
		int MouseY = static_cast<int>(std::round((delta.x / LocalPlayer.Client.Sensitivity) / 0.011f));

		if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
			mouse_event(MOUSEEVENTF_MOVE, MouseX, -MouseY, NULL, NULL);
			OldPunch = punchNow;
		}
	} else {
		OldPunch = Vec2{ 0, 0 };
	}
}
