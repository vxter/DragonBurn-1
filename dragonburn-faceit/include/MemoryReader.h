#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <random>

namespace faceit {

class MemoryReader {
public:
    MemoryReader();
    ~MemoryReader();

    // Core memory operations
    bool ReadMemory(uintptr_t address, void* buffer, size_t size);
    bool WriteMemory(uintptr_t address, const void* buffer, size_t size);
    
    // Game-specific operations
    bool ReadGameState();
    bool GetPlayerPositions(std::vector<float>& positions);
    bool GetBombLocation(float& x, float& y, float& z, bool& isPlanted);
    
    // Detection evasion
    void ApplyJitter();
    void RandomizeAccessPattern();
    void ObfuscateCallStack();
    
    // Status
    bool IsConnected() const { return m_connected; }
    const std::string& GetLastError() const { return m_lastError; }

private:
    // Process/handle management
    bool InitializeProcessHandle();
    void CleanupProcessHandle();
    
    // Memory access abstraction
    bool DirectMemoryRead(uintptr_t address, void* buffer, size_t size);
    bool KernelModuleRead(uintptr_t address, void* buffer, size_t size);
    bool DebuggerRead(uintptr_t address, void* buffer, size_t size);
    
    // Evasion patterns
    void SleeperThread();
    void RandomizeReadOrder();
    void ObfuscatePatterns();
    
    // State
    bool m_connected = false;
    std::string m_lastError;
    uintptr_t m_gameBaseAddress = 0;
    
    // Detection evasion
    std::mt19937 m_rng;
    std::chrono::steady_clock::time_point m_lastAccessTime;
    int m_accessCount = 0;
};

} // namespace faceit
