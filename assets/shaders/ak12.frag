#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// =====================================================================
// [ BAB: MODEL LOADING - MATERIALS & TEXTURES ]
// Menerima tekstur warna dan properti material dari library Assimp
// =====================================================================
uniform sampler2D texture_diffuse1;
uniform bool has_diffuse;
uniform vec3 mat_diffuse;
uniform float mat_diffuse_alpha;

// =====================================================================
// [ BAB: LIGHTING - BASIC LIGHTING (PHONG) ]
// Properti cahaya matahari/lingkungan utama
// =====================================================================
vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
vec3 lightColor = vec3(1.3, 1.3, 1.3);          
float ambientStrength = 0.7;

// =====================================================================
// [ FITUR CUSTOM: LOGIKA VISUAL TUGAS AKHIR ]
// =====================================================================
uniform bool isBillboardFlash; // Penanda apakah piksel ini adalah api ledakan
uniform float flashOpacity;    // Transparansi api yang dikontrol dari C++
uniform vec3 flashLightPos;    // Titik pusat sumber cahaya dinamis ledakan

void main() {
    // =====================================================================
    // [ FITUR CUSTOM: PROCEDURAL 2D RENDERING ]
    // Menggambar bentuk bintang/api murni menggunakan matematika GPU
    // =====================================================================
    if (isBillboardFlash) {
        if (flashOpacity < 0.01) discard; // Optimasi rendering GPU

        vec2 uv = TexCoords - vec2(0.5); 
        float radius = length(uv);
        float angle = atan(uv.y, uv.x);
        
        // Membentuk pola 4 ujung tajam (star shape)
        float star = (abs(cos(angle * 2.0)) * 0.15) + 0.05;
        if (radius > star) discard; 
        
        // Fading gradasi warna api berdasarkan jarak dari pusat
        float intensity = 1.0 - (radius / 0.2);
        vec3 warnaApi = vec3(1.0, 0.9 * intensity + 0.1, 0.5 * intensity);
        
        FragColor = vec4(warnaApi * intensity, intensity * flashOpacity);
        return;
    }

    // =====================================================================
    // [ BAB: LIGHTING - BASIC LIGHTING (PHONG) ]
    // =====================================================================
    vec4 texColor = has_diffuse ? texture(texture_diffuse1, TexCoords) : vec4(mat_diffuse, mat_diffuse_alpha);
    if (has_diffuse && length(texColor.rgb) < 0.05) texColor = vec4(0.6, 0.6, 0.6, 1.0);
    if (texColor.a < 0.1) discard;

    // 1. Hitung Ambient (Cahaya dasar ruangan)
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    
    // 2. Hitung Diffuse (Cahaya terarah dari matahari)
    float diffDir = max(dot(norm, lightDir), 0.0);
    vec3 diffuseDir = diffDir * lightColor;

    // =====================================================================
    // [ BAB: LIGHTING - LIGHT CASTERS (POINT LIGHT) ]
    // Cahaya ledakan dinamis yang memantul ke bodi besi senjata
    // =====================================================================
    vec3 flashColor = vec3(2.0, 1.2, 0.2); // Warna oranye kemerahan mesiu
    vec3 flashDir = normalize(flashLightPos - FragPos);
    float flashDiff = max(dot(norm, flashDir), 0.0);
    
    // Attenuation (Pelemahan Cahaya): Semakin jauh dari laras, pantulan makin pudar
    float distance = length(flashLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 2.0 * distance + 5.0 * (distance * distance));
    
    // Gabungkan dengan opacity dari mekanika penembakan di C++
    vec3 dynamicFlashLight = flashDiff * flashColor * attenuation * flashOpacity * 5.0;

    // Output Warna Final (Gabungan Cahaya Alam + Cahaya Ledakan + Tekstur)
    vec3 finalColor = (ambient + diffuseDir + dynamicFlashLight) * texColor.rgb;
    FragColor = vec4(finalColor, texColor.a);
}