#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

// samplerCube adalah tipe data khusus GPU untuk membaca 6 gambar sekaligus
uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}