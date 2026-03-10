cmake_minimum_required(VERSION 3.16)
project(dragonburn-faceit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(dragonburn-faceit
    src/MemoryReader.cpp
    src/AntiCheatEvasion.cpp
    src/main.cpp
)

# Include directories
target_include_directories(dragonburn-faceit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Windows-specific libraries
target_link_libraries(dragonburn-faceit 
    kernel32
    user32
    psapi
)

# Output directory
set_target_properties(dragonburn-faceit PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Optional: Kernel module
if(ENABLE_KERNEL_MODULE)
    add_subdirectory(kernel)
endif()
