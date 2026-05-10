#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform bool has_diffuse;
uniform vec3 mat_diffuse;
uniform float mat_diffuse_alpha;

// Kontrol Api 2D
uniform bool isBillboardFlash;
uniform float flashOpacity; 

// Kordinat Lampu Ledakan (Dikirim dari C++)
uniform vec3 flashLightPos; 

vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
vec3 lightColor = vec3(1.3, 1.3, 1.3);          
float ambientStrength = 0.7;                    

void main() {
    // --- FITUR KANVAS API 2D ---
    if (isBillboardFlash) {
        if (flashOpacity < 0.01) discard;

        vec2 uv = TexCoords - vec2(0.5); 
        float radius = length(uv);
        float angle = atan(uv.y, uv.x);
        float star = (abs(cos(angle * 2.0)) * 0.15) + 0.05; 
        if (radius > star) discard; 
        
        float intensity = 1.0 - (radius / 0.2);
        vec3 warnaApi = vec3(1.0, 0.9 * intensity + 0.1, 0.5 * intensity);
        
        FragColor = vec4(warnaApi * intensity, intensity * flashOpacity);
        return;
    }

    // --- RENDER SENJATA & PANTULAN CAHAYA ---
    vec4 texColor = has_diffuse ? texture(texture_diffuse1, TexCoords) : vec4(mat_diffuse, mat_diffuse_alpha);
    if (has_diffuse && length(texColor.rgb) < 0.05) texColor = vec4(0.6, 0.6, 0.6, 1.0);
    if (texColor.a < 0.1) discard;

    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    
    // 1. Cahaya Alam (Matahari / Lingkungan)
    float diffDir = max(dot(norm, lightDir), 0.0);
    vec3 diffuseDir = diffDir * lightColor;

    // 2. CAHAYA DINAMIS LEDAKAN SENJATA (POINT LIGHT)
    vec3 flashColor = vec3(2.0, 1.2, 0.2); // Warna oranye kemerahan khas mesiu
    vec3 flashDir = normalize(flashLightPos - FragPos);
    float flashDiff = max(dot(norm, flashDir), 0.0);
    
    // Hitung jarak agar cahayanya memudar di area popor/belakang senjata
    float distance = length(flashLightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 2.0 * distance + 5.0 * (distance * distance));
    
    // Gabungkan tingkat terang dengan flashOpacity. Dikali 5.0 agar pantulannya sangat kuat!
    vec3 dynamicFlashLight = flashDiff * flashColor * attenuation * flashOpacity * 5.0;

    // 3. Gabungkan Cahaya Alam + Cahaya Ledakan ke Warna Tekstur
    vec3 finalColor = (ambient + diffuseDir + dynamicFlashLight) * texColor.rgb;
    FragColor = vec4(finalColor, texColor.a);
}