# DragonBurn-FACEIT
**Whitehack Competition - Anti-Cheat Evasion Research**

Standalone memory reading framework for testing FACEIT anti-cheat detection capabilities.

## Structure
```
dragonburn-faceit/
├── src/              - Userspace memory reading
├── include/          - Headers
├── kernel/           - Optional kernel module for memory access
├── docker/           - Docker container setup
└── data/             - Shared data files
```

## Components

### 1. Userspace Memory Reader (`src/MemoryReader.cpp`)
- Direct memory read patterns
- Detection evasion techniques
- Jitter/sleep patterns
- Obfuscation layers

### 2. Kernel Module (`kernel/driver.c`) [Optional]
- Elevated memory access
- Pattern obfuscation
- Can be removed if detected

### 3. Webradar Integration
- Output game state to network socket
- Visualization via webradar.html

### 4. Docker Container
- Isolated memory reading service
- Network communication with game

## Usage

**Userspace only (safer):**
```
cmake -B build
cmake --build build
./build/dragonburn-faceit.exe
```

**With kernel module:**
```
# Load driver
sc create dragonburn binPath= "C:\path\to\driver.sys"
sc start dragonburn

# Run userspace component
./build/dragonburn-faceit.exe
```

**Docker:**
```
docker build -t dragonburn-faceit .
docker run --isolation=process dragonburn-faceit
```

## Detection Evasion Techniques

- [ ] Random memory access patterns
- [ ] Anti-debugger checks
- [ ] Timing-based AC evasion
- [ ] Kernel callback hiding
- [ ] Driver signature obfuscation
- [ ] PEB/TEB manipulation
- [ ] Thread injection patterns

## FACEIT AC Testing Matrix

| Method | Detection Risk | Effectiveness | Notes |
|--------|---|---|---|
| Direct memory read | High | High | Most direct, easily detected |
| Kernel module | Medium | Very High | Can be removed mid-session |
| Hardware inspection | Low | Medium | Requires special hardware |
| Debugger API | High | Medium | Explicitly blocked by AC |
| Driver callbacks | Medium | High | Requires careful obfuscation |

---
**For legitimate security research only**
