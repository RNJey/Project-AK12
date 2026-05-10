cmake_minimum_required(VERSION 3.10)
project(BlackHole_UAS)

set(CMAKE_CXX_STANDARD 17)

# Mencari library yang sudah diinstal via MSYS2 pacman
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glm REQUIRED) # <--- TAMBAHKAN BARIS INI

# Menambahkan folder include agar compiler tahu letak header file
include_directories(include)
include_directories(src/imgui) 

# Mengumpulkan semua file .cpp dan .c yang akan di-compile
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# Membuat file executable 
add_executable(BlackHole_UAS ${SOURCES}
    src/main.cpp
    src/glad.c
    src/cgltf.cpp
)

# Menggabungkan (linking) library ke executable
target_link_libraries(BlackHole_UAS glfw assimp glm::glm)