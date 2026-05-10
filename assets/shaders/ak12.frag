#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// =====================================================================
// [ LEARN OPENGL ] VARIABEL STANDAR MATERIAL & TEKSTUR
// =====================================================================
uniform sampler2D texture_diffuse1;
uniform bool has_diffuse;
uniform vec3 mat_diffuse;
uniform float mat_diffuse_alpha;

vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
vec3 lightColor = vec3(1.3, 1.3, 1.3);          
float ambientStrength = 0.7;

// =====================================================================
// [ FITUR CUSTOM ] KONTROL VISUAL LEDAKAN (DARI C++)
// =====================================================================
uniform bool isBillboardFlash; // Penanda objek adalah api
uniform float flashOpacity;    // Tingkat transparansi api
uniform vec3 flashLightPos;    // Titik pusat sumber cahaya ledakan

void main() {
    // =====================================================================
    // [ FITUR CUSTOM ] PROSEDURAL MUZZLE FLASH 2D
    // =====================================================================
    // Menggambar bentuk bintang/api secara matematis murni menggunakan GPU
    if (isBillboardFlash) {
        if (flashOpacity < 0.01) discard; // Optimasi GPU

        vec2 uv = TexCoords - vec2(0.5); 
        float radius = length(uv);
        float angle = atan(uv.y, uv.x);
        
        // Membentuk pola 4 ujung tajam (star)
        float star = (abs(cos(angle * 2.0)) * 0.15) + 0.05; 
        if (radius > star) discard; 
        
        // Fading gradasi warna api berdasarkan jarak dari pusat
        float intensity = 1.0 - (radius / 0.2);
        vec3 warnaApi = vec3(1.0, 0.9 * intensity + 0.1, 0.5 * intensity);
        
        FragColor = vec4(warnaApi * intensity, intensity * flashOpacity);
        return;
    }

    // =====================================================================
    // [ LEARN OPENGL ] PERHITUNGAN PENCAHAYAAN DASAR (PHONG LIGHTING)
    // =====================================================================
    vec4 texColor = has_diffuse ? texture(texture_diffuse1, TexCoords) : vec4(mat_diffuse, mat_diffuse_alpha);
    if (has_diffuse && length(texColor.rgb) < 0.05) texColor = vec4(0.6, 0.6, 0.6, 1.0);
    if (texColor.a < 0.1) discard;

    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    
    // 1. Cahaya Lingkungan Standar (Directional Light)
    float diffDir = max(dot(norm, lightDir), 0.0);
    vec3 diffuseDir = diffDir * lightColor;

    // =====================================================================
    // [ FITUR CUSTOM ] CAHAYA TITIK DINAMIS (DYNAMIC POINT LIGHT)
    // =====================================================================
    // Cahaya ini akan memantul ke bodi besi senjata saat menembak
    vec3 flashColor = vec3(2.0, 1.2, 0.2); // Warna oranye kemerahan
    vec3 flashDir = normalize(flashLightPos - FragPos);
    float flashDiff = max(dot(norm, flashDir), 0.0);
    
    // Attenuation: Semakin jauh dari laras, cahaya di bodi senjata semakin lemah
    float distance = length(flashLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 2.0 * distance + 5.0 * (distance * distance));
    
    // Menggabungkan intensitas berdasarkan variabel flashOpacity dari C++
    vec3 dynamicFlashLight = flashDiff * flashColor * attenuation * flashOpacity * 5.0;

    // Output Warna Final
    vec3 finalColor = (ambient + diffuseDir + dynamicFlashLight) * texColor.rgb;
    FragColor = vec4(finalColor, texColor.a);
}