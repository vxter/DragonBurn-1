#include "../include/MemoryReader.h"
#include <windows.h>
#include <thread>
#include <chrono>

namespace faceit {

MemoryReader::MemoryReader() : m_rng(std::random_device{}()) {
    m_lastAccessTime = std::chrono::steady_clock::now();
}

MemoryReader::~MemoryReader() {
    CleanupProcessHandle();
}

bool MemoryReader::ReadMemory(uintptr_t address, void* buffer, size_t size) {
    // Apply evasion techniques
    ApplyJitter();
    RandomizeAccessPattern();
    
    // Try multiple read methods with fallback
    if (DirectMemoryRead(address, buffer, size)) {
        m_accessCount++;
        return true;
    }
    
    if (KernelModuleRead(address, buffer, size)) {
        m_accessCount++;
        return true;
    }
    
    if (DebuggerRead(address, buffer, size)) {
        m_accessCount++;
        return true;
    }
    
    m_lastError = "Failed to read memory at " + std::to_string(address);
    return false;
}

bool MemoryReader::WriteMemory(uintptr_t address, const void* buffer, size_t size) {
    // Similar to ReadMemory but for writes
    ApplyJitter();
    return DirectMemoryRead(address, (void*)buffer, size);
}

bool MemoryReader::ReadGameState() {
    // Placeholder for reading full game state
    // This would integrate with actual game offsets
    return true;
}

bool MemoryReader::GetPlayerPositions(std::vector<float>& positions) {
    // Read player positions from game memory
    // Randomize access order to avoid pattern detection
    RandomizeReadOrder();
    
    positions.clear();
    // TODO: Implement actual position reading
    
    return true;
}

bool MemoryReader::GetBombLocation(float& x, float& y, float& z, bool& isPlanted) {
    // Read bomb state from game memory
    // Use obfuscated patterns to avoid AC detection
    ObfuscatePatterns();
    
    x = y = z = 0.0f;
    isPlanted = false;
    // TODO: Implement actual bomb reading
    
    return true;
}

void MemoryReader::ApplyJitter() {
    // Add random delays between reads to defeat timing-based detection
    std::uniform_int_distribution<> dist(1, 100);
    int jitterMs = dist(m_rng);
    
    // Adaptive jitter based on recent access patterns
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAccessTime).count();
    
    if (elapsed < 10) {
        // Too fast, add more jitter
        jitterMs += 50;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(jitterMs));
    m_lastAccessTime = now;
}

void MemoryReader::RandomizeAccessPattern() {
    // Randomize which read method to use
    std::uniform_int_distribution<> dist(0, 2);
    int method = dist(m_rng);
    
    // This helps defeat pattern matching in AC systems
    if (m_accessCount % 10 == 0) {
        ApplyJitter(); // Extra jitter every 10 reads
    }
}

void MemoryReader::ObfuscateCallStack() {
    // Add dummy function calls to obscure the call stack
    volatile int dummy = 0;
    for (int i = 0; i < 100; i++) {
        dummy += i;
    }
}

bool MemoryReader::InitializeProcessHandle() {
    // Get CS2 process handle
    // This is a simplification; real implementation would handle multiple methods
    
    DWORD processId = 0;
    HWND csWindow = FindWindowA(NULL, "Counter-Strike 2");
    
    if (!csWindow) {
        m_lastError = "CS2 window not found";
        return false;
    }
    
    GetWindowThreadProcessId(csWindow, &processId);
    if (!processId) {
        m_lastError = "Failed to get process ID";
        return false;
    }
    
    m_connected = true;
    return true;
}

void MemoryReader::CleanupProcessHandle() {
    m_connected = false;
}

bool MemoryReader::DirectMemoryRead(uintptr_t address, void* buffer, size_t size) {
    // Direct memory read via ReadProcessMemory
    // This is the most detectable method
    
    if (!m_connected && !InitializeProcessHandle()) {
        return false;
    }
    
    // TODO: Implement with actual process handle
    return false;
}

bool MemoryReader::KernelModuleRead(uintptr_t address, void* buffer, size_t size) {
    // Kernel module memory read
    // Less detectable if properly obfuscated
    
    // TODO: Implement kernel communication
    return false;
}

bool MemoryReader::DebuggerRead(uintptr_t address, void* buffer, size_t size) {
    // Debugger API read (most likely to be detected)
    // Try as last resort
    
    // TODO: Implement debugger API
    return false;
}

void MemoryReader::SleeperThread() {
    // Background thread to add noise and make behavior less predictable
    while (m_connected) {
        ApplyJitter();
    }
}

void MemoryReader::RandomizeReadOrder() {
    // Randomize the order of operations
    std::vector<int> indices = {0, 1, 2, 3, 4};
    std::shuffle(indices.begin(), indices.end(), m_rng);
}

void MemoryReader::ObfuscatePatterns() {
    // Add multiple layers of obfuscation
    ObfuscateCallStack();
    ApplyJitter();
    RandomizeAccessPattern();
}

} // namespace faceit
