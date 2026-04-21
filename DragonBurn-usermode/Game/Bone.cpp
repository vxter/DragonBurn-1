#include "Bone.h"


bool CBone::UpdateAllBoneData(const DWORD64& EntityPawnAddress) {
    if (!EntityPawnAddress) return false;
    this->EntityPawnAddress = EntityPawnAddress;

    uintptr_t gameSceneNodeAddr;
    if (!memoryManager.ReadMemory(EntityPawnAddress + Offset.Pawn.GameSceneNode, gameSceneNodeAddr))
        return false;

    this->GameSceneNode = gameSceneNodeAddr;

    constexpr size_t NUM_BONES = 30;

    auto tryReadCandidate = [&](DWORD boneArrayOffset) -> bool {
        uintptr_t boneArrayAddr = 0;
        if (!memoryManager.ReadMemory(gameSceneNodeAddr + boneArrayOffset, boneArrayAddr) || boneArrayAddr == 0)
            return false;

        // Read bone data (position + rotation)
        CBoneData IData[NUM_BONES];
        if (!memoryManager.ReadMemory(boneArrayAddr, IData, NUM_BONES * sizeof(CBoneData)))
            return false;

        // Read original bone data (position + scale)
        BoneJointData originalData[NUM_BONES];
        if (!memoryManager.ReadMemory(boneArrayAddr, originalData, NUM_BONES * sizeof(BoneJointData)))
            return false;

        std::vector<BoneJointPos> tmpBonePos;
        std::vector<CBoneData> tmpIBone;
        tmpBonePos.reserve(NUM_BONES);
        tmpIBone.reserve(NUM_BONES);

        size_t visibleCount = 0;

		for (size_t i = 0; i < NUM_BONES; ++i) {
			Vec2 screenPos;
			bool visible = gGame.View.WorldToScreen(originalData[i].Pos, screenPos);
			if (visible)
				visible = (screenPos.x >= 0.0f && screenPos.x <= Gui.Window.Size.x &&
					screenPos.y >= 0.0f && screenPos.y <= Gui.Window.Size.y);
			// Bone 27 produces incorrect box stretching.
			if (i == 27)
				visible = false;
			if (visible)
				++visibleCount;

            tmpBonePos.push_back({ originalData[i].Pos, screenPos, visible });
            tmpIBone.push_back({
                originalData[i].Pos,  // Location
                originalData[i].Scale,  // Scale
                IData[i].Rotation  // Rotation
            });
        }

        // If we projected only a single/incorrect point, ESP boxes can grow along the LOS line.
        // Pick the candidate that produces enough visible bones.
        if (visibleCount < 3)
            return false;

        BonePosList.swap(tmpBonePos);
        IBoneData.swap(tmpIBone);

        return true;
    };

    // Try the configured offset and nearby adjustments.
    const DWORD base = Offset.Pawn.BoneArray;
    bool ok = tryReadCandidate(base) || tryReadCandidate(base - 0x80) || tryReadCandidate(base + 0x80);
    if (!ok)
        return false;

    return true;
}

/*
std::optional<CBoneData> CBone::GetBoneData(int index) const {
    if (index < 0 || index >= IBoneData.size()) {
        // Log::Debug("[DEBUG] FAIL GetBoneData: Index out of bounds");
        return std::nullopt;
    }
    return IBoneData[index];
}
*/

bool CBone::UpdateAllBoneDataBatch(const DWORD64& EntityPawnAddress) {
	if (EntityPawnAddress == 0)
		return false;

	this->EntityPawnAddress = EntityPawnAddress;

	// BATCH READ 1: Get dependent addresses
	std::vector<std::pair<DWORD64, SIZE_T>> batch1Requests = {
		{EntityPawnAddress + Offset.Pawn.GameSceneNode, sizeof(DWORD64)},  // GameSceneNodeAddr
	};

	std::vector<BYTE> batch1Buffer(sizeof(DWORD64));

	if (!memoryManager.BatchReadMemory(batch1Requests, batch1Buffer.data())) {
		return false;
	}

	// Extract GameSceneNode address
	DWORD64 GameSceneNodeAddr;
	memcpy(&GameSceneNodeAddr, batch1Buffer.data(), sizeof(DWORD64));

	if (GameSceneNodeAddr == 0) return false;
	this->GameSceneNode = GameSceneNodeAddr;

	// BATCH READ 2: Get BoneArray address
	std::vector<std::pair<DWORD64, SIZE_T>> batch2Requests = {
		{GameSceneNodeAddr + Offset.Pawn.BoneArray, sizeof(DWORD64)}  // BoneArrayAddress
	};

	std::vector<BYTE> batch2Buffer(sizeof(DWORD64));

	if (!memoryManager.BatchReadMemory(batch2Requests, batch2Buffer.data())) {
		return false;
	}

	// Extract BoneArray address
	DWORD64 BoneArrayAddress;
	memcpy(&BoneArrayAddress, batch2Buffer.data(), sizeof(DWORD64));

	if (BoneArrayAddress == 0) return false;

	// BATCH READ 3: Read all bone data at once
	constexpr size_t NUM_BONES = 30;
	std::vector<std::pair<DWORD64, SIZE_T>> batch3Requests;
	batch3Requests.reserve(NUM_BONES);

	// Create requests for each bone (each bone is 32 bytes apart)
	for (size_t i = 0; i < NUM_BONES; ++i) {
		batch3Requests.push_back({ BoneArrayAddress + (i * 32), sizeof(BoneJointData) });
	}

	// Calculate total buffer size
	SIZE_T total_size = NUM_BONES * sizeof(BoneJointData);
	std::vector<BYTE> batch3Buffer(total_size);

	// Perform batch read for all bones
	if (!memoryManager.BatchReadMemory(batch3Requests, batch3Buffer.data())) {
		return false;
	}

	// Clear both lists
	BonePosList.clear();
	IBoneData.clear();
	BonePosList.reserve(NUM_BONES);
	IBoneData.reserve(NUM_BONES);

	// Extract bone data from buffer
	SIZE_T offset = 0;
	for (size_t i = 0; i < NUM_BONES; ++i) {
		BoneJointData bone;
		memcpy(&bone, batch3Buffer.data() + offset, sizeof(BoneJointData));
		offset += sizeof(BoneJointData);

		Vec2 ScreenPos;
			bool IsVisible = false;
			if (gGame.View.WorldToScreen(bone.Pos, ScreenPos))
				IsVisible = (ScreenPos.x >= 0.0f && ScreenPos.x <= Gui.Window.Size.x &&
					ScreenPos.y >= 0.0f && ScreenPos.y <= Gui.Window.Size.y);
			if (i == 27)
				IsVisible = false;

		// Original bone position data
		this->BonePosList.push_back({ bone.Pos, ScreenPos, IsVisible });

		// bone data
		CBoneData IBone;
		IBone.Location = bone.Pos;
		IBone.Scale = bone.Scale;
		this->IBoneData.push_back(IBone);
	}

	return !BonePosList.empty();
}
