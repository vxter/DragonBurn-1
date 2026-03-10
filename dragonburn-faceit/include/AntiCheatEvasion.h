#pragma once
#include <cstdint>
#include <vector>

namespace faceit::evasion {

// Detection patterns to avoid
class AntiDetection {
public:
    // Timing-based evasion
    static void RandomSleep(int minMs = 1, int maxMs = 50);
    static void AdaptiveDelay();
    
    // Pattern obfuscation
    static void ObfuscateMemoryAccess();
    static void RandomizeCallOrder();
    
    // Anti-debugger
    static bool DetectDebugger();
    static bool CheckPEB();
    
    // Anti-analysis
    static void HideThreads();
    static void FakePEBData();
};

// Detection patterns FACEIT AC likely uses
class FACEITPatterns {
public:
    // Common signatures to avoid
    static bool HasSuspiciousReadPattern();
    static bool HasConsecutiveMemoryAccess();
    static bool HasKnownCheatSignature();
    
    // Randomization to defeat pattern matching
    static void RandomizeAccessSize();
    static void RandomizeAccessFrequency();
    static void InterleaveFakeReads();
};

// Kernel module integration
class KernelBridge {
public:
    static bool LoadDriver(const std::string& driverPath);
    static bool UnloadDriver();
    static bool IsDriverLoaded();
    
    // Kernel-level memory access (less detectable)
    static bool KernelRead(uintptr_t address, void* buffer, size_t size);
    static bool KernelWrite(uintptr_t address, const void* buffer, size_t size);
};

} // namespace faceit::evasion
