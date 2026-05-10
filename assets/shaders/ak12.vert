#version 330 core

// =====================================================================
// [ BAB: GETTING STARTED - SHADERS ] 
// Menerima data vertex dari CPU (C++) melalui layout location
// =====================================================================
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out vec3 LocalPos;

// =====================================================================
// [ BAB: GETTING STARTED - COORDINATE SYSTEMS ]
// Matriks Transformasi 3D (Model, View, Projection / MVP)
// =====================================================================
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;
    LocalPos = aPos;
    
    // =====================================================================
    // [ BAB: LIGHTING - BASIC LIGHTING ]
    // Mentranslasikan posisi vertex ke World Space untuk hitung cahaya
    // =====================================================================
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Menghitung Normal Matrix agar arah pantulan cahaya tidak rusak saat objek diskala/rotasi
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Mengkonversi kordinat akhir ke ruang layar monitor (Clip Space)
    gl_Position = projection * view * vec4(FragPos, 1.0);
}