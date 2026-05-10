#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out vec3 LocalPos;

// =====================================================================
// [ LEARN OPENGL ] MATRIKS TRANSFORMASI (MVP)
// =====================================================================
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;
    LocalPos = aPos;
    
    // Mentranslasikan posisi 3D ke kordinat dunia (World Space) untuk perhitungan cahaya
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Mengkonversi ke ruang proyeksi layar
    gl_Position = projection * view * vec4(FragPos, 1.0);
}