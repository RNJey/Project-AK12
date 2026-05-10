#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // Menghilangkan translasi pada view matrix agar skybox selalu mengikuti kamera
    mat4 rotView = mat4(mat3(view)); 
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    // Mengatur kedalaman ke nilai maksimal (1.0) agar skybox selalu di belakang objek lain
    gl_Position = pos.xyww;
}