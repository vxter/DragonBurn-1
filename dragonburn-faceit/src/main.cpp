#include "../include/MemoryReader.h"
#include <iostream>
#include <thread>
#include <chrono>

void PrintBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════╗
║      DragonBurn-FACEIT Whitehack Competition        ║
║         Anti-Cheat Evasion Research Tool            ║
╚══════════════════════════════════════════════════════╝
    )" << std::endl;
}

void RadarThread(faceit::MemoryReader& reader) {
    std::cout << "[*] Starting radar thread..." << std::endl;
    
    while (reader.IsConnected()) {
        // Read game state and output to webradar
        std::vector<float> playerPositions;
        float bombX, bombY, bombZ;
        bool bombPlanted;
        
        if (reader.GetPlayerPositions(playerPositions)) {
            std::cout << "[+] Read " << playerPositions.size() / 3 << " players" << std::endl;
        }
        
        if (reader.GetBombLocation(bombX, bombY, bombZ, bombPlanted)) {
            std::cout << "[+] Bomb: (" << bombX << ", " << bombY << ", " << bombZ << ") Planted: " << bombPlanted << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void EvasionThread() {
    std::cout << "[*] Starting evasion thread..." << std::endl;
    
    // Background noise to make behavior less predictable
    while (true) {
        // Add random operations to disguise memory reads
        volatile int dummy = 0;
        for (int i = 0; i < 1000000; i++) {
            dummy += i;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    PrintBanner();
    
    std::cout << "[*] Initializing memory reader..." << std::endl;
    faceit::MemoryReader reader;
    
    if (!reader.IsConnected()) {
        std::cerr << "[!] Failed to connect to game process" << std::endl;
        std::cerr << "[!] Error: " << reader.GetLastError() << std::endl;
        return 1;
    }
    
    std::cout << "[+] Successfully connected to CS2" << std::endl;
    
    // Start threads
    std::thread radar(RadarThread, std::ref(reader));
    std::thread evasion(EvasionThread);
    
    std::cout << "[+] Threads started" << std::endl;
    std::cout << "[+] Listening on localhost:8080" << std::endl;
    std::cout << "[+] Press Ctrl+C to exit" << std::endl;
    
    // Main loop - keep program running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
