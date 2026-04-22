#pragma once
#include "Aimbot.h"

namespace RCS
{
	inline int RCSBullet = 1;
	inline Vec2 RCSScale = { 1.4f,1.4f };

	// Per-weapon full clip calibration (aim-punch table).
	struct WeaponProfile {
		// bullets[shots - firstShot] gives aimPunchAngle at that shot count.
		// firstShot is the absolute ShotsFired value for bullets[0].
		uint32_t firstShot = 1;
		// bullets[i] => aimPunchAngle for shot i+1
		std::vector<Vec2> bullets;
	};

	// Calibration/recording state (driven by GUI, executed from RCS tick).
	inline bool CalibrationMode = false; // shows calibration UI
	inline bool CalibrationRecording = false;
	inline std::string CalibrationWeapon = "";
	inline bool CalibrationAutoRun = false;
	inline bool PendingStartCalibration = false;
	inline bool PendingStopAndSave = false;
	inline bool PendingDiscard = false;

	// UI -> recorder requests
	void RequestStartCalibration();
	void RequestStartCalibrationAuto();
	void RequestStopAndSave();
	void RequestDiscard();
	bool IsCalibrating();

	// Playback helpers
	bool GetProfileAimPunch(const CEntity& local, Vec2& outAimPunch);
	bool GetProfileForWeapon(const std::string& weaponName, WeaponProfile& outProfile);

	void UpdateAngles(const CEntity&, Vec2&);
	void RecoilControl(CEntity);
}
