#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib> // Untuk fungsi rand()

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//Audio
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(std::vector<std::string> faces);

// --- STATE-MACHINE INSPECTOR KAMERA ---
enum CameraState { 
    STATE_FREE, 
    STATE_INSPECT_BARREL, 
    STATE_INSPECT_MAGAZINE, 
    STATE_INSPECT_POPOR, 
    STATE_INSPECT_GRIP, 
    STATE_INSPECT_SCOPE, 
    STATE_SHOOTING_MODE 
};
CameraState currentCamState = STATE_FREE;

// Target posisi kamera & sudut pandang (Yaw/Pitch) untuk masing-masing bagian senjata
glm::vec3 targetCamPos = glm::vec3(0.0f, 0.0f, 5.0f);
float targetYaw = -90.0f;
float targetPitch = 0.0f;

// --- VARIABEL FLOATING UI & TYPEWRITER ---
std::string fullTypeText = "";
std::string currentTypeText = "";
float typeTimer = 0.0f;
int typeIndex = 0;
float typeSpeed = 0.015f; // Kecepatan ngetik (semakin kecil semakin ngebut)

ImVec2 floatingUIPos = ImVec2(50, 50); // Posisi X,Y di layar
bool showFloatingUI = false;

// --- LOGIKA SIMULASI MENEMBAK & RECOIL ---
bool isFiring = false;
float fireTimer = 0.0f;
float fireRate = 0.08f;      // Kecepatan tembakan (0.08 detik per peluru)
float weaponKickback = 0.0f; // Seberapa jauh senjata mundur saat menembak

// --- VARIABEL KONTROL API PERMANEN (PENGGANTI TIMER) ---
float flashOpacity = 0.0f;   // 0.0 = Hilang, 1.0 = Nyala Terang

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simulasi Senjata AK-12 - Tugas Akhir", NULL, NULL);
    if (window == NULL) {
        std::cout << "Gagal membuat window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    
    glEnable(GL_DEPTH_TEST);
    
    // NYALAKAN BLENDING UNTUK EFEK FADING API & UI TRANSPARAN
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader skyboxShader("../assets/shaders/skybox.vert", "../assets/shaders/skybox.frag");
    Shader ak12Shader("../assets/shaders/ak12.vert", "../assets/shaders/ak12.frag");

    // --- SETUP SKYBOX ---
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // --- SETUP MUZZLE FLASH QUAD (KANVAS API 2D) ---
    float flashVertices[] = {
        // Posisi (X, Y, Z)     // Normal           // TexCoords
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f
    };
    unsigned int flashVAO, flashVBO;
    glGenVertexArrays(1, &flashVAO);
    glGenBuffers(1, &flashVBO);
    glBindVertexArray(flashVAO);
    glBindBuffer(GL_ARRAY_BUFFER, flashVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(flashVertices), &flashVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    std::vector<std::string> faces = { "../assets/skybox/right.png", "../assets/skybox/left.png", "../assets/skybox/top.png", "../assets/skybox/bottom.png", "../assets/skybox/front.png", "../assets/skybox/back.png" };
    unsigned int cubemapTexture = loadCubemap(faces);
    skyboxShader.use();
    skyboxShader.setFloat("skybox", 0);

    // --- LOAD MODEL AK-12 ---
    Model ak12Model("../assets/ak12/ak12rend.obj"); 
    float modelScale = 0.5f; 
    float tiltX = 0.0f, tiltY = 90.0f, tiltZ = 0.0f; // Bawaan hadap samping

    // --- INISIALISASI AUDIO ENGINE ---
    ma_engine engine;
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Peringatan: Gagal memuat Audio Engine Miniaudio!\n");
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        if (deltaTime > 0.05f) {
            deltaTime = 0.05f; 
        }

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- INTERPOLASI TRANSISI KAMERA SECARA HALUS (LERPING) ---
        if (currentCamState != STATE_FREE) {
            camera.Position = glm::mix(camera.Position, targetCamPos, deltaTime * 5.0f);
            camera.Yaw = glm::mix(camera.Yaw, targetYaw, deltaTime * 5.0f);
            camera.Pitch = glm::mix(camera.Pitch, targetPitch, deltaTime * 5.0f);
            camera.updateCameraVectors(); 
        }

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // --- PROSES SIMULASI MENEMBAK & RECOIL ---
        if (isFiring) {
            fireTimer += deltaTime;
            if (fireTimer >= fireRate) {
                fireTimer = 0.0f;
                weaponKickback = 0.15f;
                
                flashOpacity = 1.0f; 

                ma_engine_play_sound(&engine, "../assets/audio/ak12-fire.wav", NULL);

                float randomPitchOffset = (rand() % 100 / 100.0f) * 1.5f; 
                float randomYawOffset = ((rand() % 100 / 50.0f) - 1.0f) * 0.5f; 
                camera.Pitch += randomPitchOffset;
                camera.Yaw += randomYawOffset;
                camera.updateCameraVectors();
            }
        }

        // --- LOGIKA REDUP API PERMANEN ---
        if (flashOpacity > 0.0f) {
            flashOpacity -= deltaTime * 8.0f; 
            if (flashOpacity < 0.0f) flashOpacity = 0.0f; 
        }

        weaponKickback = glm::mix(weaponKickback, 0.0f, deltaTime * 15.0f);

        // --- 1. RENDER MODEL SENJATA AK-12 ---
        ak12Shader.use();
        ak12Shader.setMat4("projection", projection);
        ak12Shader.setMat4("view", view);
        
        ak12Shader.setFloat("flashOpacity", flashOpacity);
        ak12Shader.setVec3("flashLightPos", glm::vec3(0.004f, 0.035f, -0.65f));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -weaponKickback)); 

        model = glm::rotate(model, glm::radians(tiltX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tiltY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tiltZ), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(modelScale)); 
        
        ak12Shader.setMat4("model", model);
        ak12Model.Draw(ak12Shader);

        // --- 2. RENDER KANVAS API (SECARA PERMANEN) ---
        glDisable(GL_DEPTH_TEST); 

        ak12Shader.use();
        ak12Shader.setBool("isBillboardFlash", true); 
        ak12Shader.setFloat("flashOpacity", flashOpacity);

        glm::mat4 flashModel = glm::mat4(1.0f);
        flashModel = glm::translate(flashModel, glm::vec3(0.004f, 0.035f, -0.65f));
        float randomRot = (rand() % 360);
        flashModel = glm::rotate(flashModel, glm::radians(randomRot), glm::vec3(0.0f, 0.0f, 1.0f));
        flashModel = glm::scale(flashModel, glm::vec3(0.15f)); 

        ak12Shader.setMat4("model", flashModel);
        
        glBindVertexArray(flashVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        ak12Shader.setBool("isBillboardFlash", false); 
        glEnable(GL_DEPTH_TEST);

        // --- 3. RENDER SKYBOX ---
        glDepthFunc(GL_LEQUAL); 
        skyboxShader.use();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); 
        skyboxShader.setMat4("view", skyboxView);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // --- 4. PANEL INTERAKTIF IMGUI ---
        ImGui::Begin("AK-12 Interactive Inspector & Simulator");

        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Fasilitas Menembak:");
        if (ImGui::Button("TEMBAK SENJATA (Klik & Tahan)", ImVec2(300, 40))) {
            isFiring = true;
        }
        if (ImGui::IsItemActive()) {
            isFiring = true; 
        } else {
            isFiring = false;
        }

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Mode Kamera Inspektor:");
        
        if (ImGui::Button("Kamera Bebas (Manual)", ImVec2(280, 25))) {
            currentCamState = STATE_FREE;
            showFloatingUI = false;
        }
        
        if (ImGui::Button("Fokus Laras (Muzzle)", ImVec2(280, 25))) {
            currentCamState = STATE_INSPECT_BARREL;
            targetCamPos = glm::vec3(-0.18f, 0.06f, -0.32f); targetYaw = -2.80f; targetPitch = -1.00f;
            
            fullTypeText = "[ FLASH SUPPRESSOR & BARREL ]\n\nLaras berlapis krom dengan 'slit flame suppressor'.\nDesain ini sangat efektif meredam kilatan api tembakan dan\nmendukung pemasangan peredam suara (LSD) secara cepat (quick-detach).";
            currentTypeText = ""; typeIndex = 0; typeTimer = 0.0f;
            floatingUIPos = ImVec2(50, 80); // Kiri Atas
            showFloatingUI = true;
        }

        if (ImGui::Button("Fokus Magazine", ImVec2(280, 25))) {
            currentCamState = STATE_INSPECT_MAGAZINE;
            targetCamPos = glm::vec3(-0.28f, -0.01f, -0.15f); targetYaw = 7.90f; targetPitch = 1.90f;
            
            fullTypeText = "[ POLYMER MAGAZINE (30 ROUNDS) ]\n\nMenampung 30 butir peluru kaliber 5.45x39mm.\nTerbuat dari polimer tahan banting dan dilengkapi dengan jendela transparan\n(visual ammunition monitor) untuk mengecek sisa peluru dengan cepat.";
            currentTypeText = ""; typeIndex = 0; typeTimer = 0.0f;
            floatingUIPos = ImVec2(50, 550); 
            showFloatingUI = true;
        }

        if (ImGui::Button("Fokus Popor (Stock)", ImVec2(280, 25))) {
            currentCamState = STATE_INSPECT_POPOR;
            targetCamPos = glm::vec3(0.20f, 0.02f, 0.10f); targetYaw = -176.50f; targetPitch = 0.40f;
            
            fullTypeText = "[ FOLDING TELESCOPIC STOCK ]\n\nPopor teleskopik lipat dengan beberapa posisi pengaturan panjang.\nPosisinya sejajar dengan laras (in-line) untuk kontrol recoil yang lebih baik,\nserta dilengkapi bantalan karet (butt plate) pelindung bahu.";
            currentTypeText = ""; typeIndex = 0; typeTimer = 0.0f;
            floatingUIPos = ImVec2(50, 80); 
            showFloatingUI = true;
        }

        if (ImGui::Button("Fokus Grip (Gagang)", ImVec2(280, 25))) {
            currentCamState = STATE_INSPECT_GRIP;
            targetCamPos = glm::vec3(0.13f, 0.01f, -0.20f); targetYaw = -176.80f; targetPitch = 1.70f;
            
            fullTypeText = "[ ERGONOMIC PISTOL GRIP ]\n\nGagang polimer ergonomis modern.\nDidesain agar penembak dapat dengan mudah menjangkau tuas pengaman\n(ambidextrous safety selector) tanpa perlu melepaskan genggaman tangan.";
            currentTypeText = ""; typeIndex = 0; typeTimer = 0.0f;
            floatingUIPos = ImVec2(1200, 750);
            showFloatingUI = true;
        }

        if (ImGui::Button("Fokus Optik (Scope)", ImVec2(280, 25))) {
            currentCamState = STATE_INSPECT_SCOPE;
            targetCamPos = glm::vec3(0.09f, 0.17f, -0.19f); targetYaw = -217.60f; targetPitch = -27.60f;
            
            fullTypeText = "[ PICATINNY RAIL & OPTICS ]\n\nDilengkapi rel Picatinny kokoh di atas penutup receiver (receiver cover).\nMendukung pemasangan berbagai optik seperti Red Dot (Collimator),\nScope teleskopik, maupun pembidik malam hari (Night Vision).";
            currentTypeText = ""; typeIndex = 0; typeTimer = 0.0f;
            floatingUIPos = ImVec2(50, 100); // Kiri Atas
            showFloatingUI = true;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Mode Simulasi Pertempuran:");
        if (ImGui::Button("MODE MENEMBAK (Aim Down Sights)", ImVec2(280, 35))) {
            currentCamState = STATE_SHOOTING_MODE;
            targetCamPos = glm::vec3(-0.09f, 0.12f, 0.14f); targetYaw = -93.53f; targetPitch = -7.57f;
            showFloatingUI = false;
        }

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        // --- 5. LOGIKA & RENDER FLOATING TYPEWRITER UI (DENGAN OUTLINE) ---
        if (showFloatingUI) {
            // Update Teks Typewriter
            if (typeIndex < fullTypeText.length()) {
                typeTimer += deltaTime;
                if (typeTimer >= typeSpeed) {
                    typeTimer = 0.0f;
                    typeIndex++;
                    currentTypeText = fullTypeText.substr(0, typeIndex); 
                }
            }

            // GABUNGKAN TEKS DENGAN KURSOR BERKEDIP
            // Ini membuat kursor (_) menyatu dengan teks utamanya
            std::string displayText = currentTypeText;
            if (typeIndex >= fullTypeText.length()) {
                if ((int)(glfwGetTime() * 2) % 2 == 0) displayText += "_"; 
            }

            // Render Window Tembus Pandang
            ImGui::SetNextWindowPos(floatingUIPos); 
            ImGuiWindowFlags floatingFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | 
                                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
            
            ImGui::Begin("FloatingPanel", NULL, floatingFlags);
            ImGui::SetWindowFontScale(1.3f); // Besarkan font sedikit
            
            // --- TRIK MULTI-PASS RENDERING UNTUK OUTLINE ---
            ImVec2 pos = ImGui::GetCursorPos(); // Simpan kordinat asli ImGui
            float offset = 1.5f; // Ketebalan outline hitam (Bisa dinaikkan ke 2.0f kalau kurang tebal)

            // 1. Gambar Teks Hitam (Geser ke 4 penjuru)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Set warna Hitam
            ImGui::SetCursorPos(ImVec2(pos.x - offset, pos.y - offset)); ImGui::Text("%s", displayText.c_str()); // Kiri Atas
            ImGui::SetCursorPos(ImVec2(pos.x + offset, pos.y - offset)); ImGui::Text("%s", displayText.c_str()); // Kanan Atas
            ImGui::SetCursorPos(ImVec2(pos.x - offset, pos.y + offset)); ImGui::Text("%s", displayText.c_str()); // Kiri Bawah
            ImGui::SetCursorPos(ImVec2(pos.x + offset, pos.y + offset)); ImGui::Text("%s", displayText.c_str()); // Kanan Bawah
            ImGui::PopStyleColor(); // Lepas warna hitam

            // 2. Gambar Teks Putih (Tepat di tengah)
            ImGui::SetCursorPos(pos); // Kembalikan kursor ke tengah
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Set warna Putih
            ImGui::Text("%s", displayText.c_str());
            ImGui::PopStyleColor(); // Lepas warna putih
            // -----------------------------------------------

            ImGui::SetWindowFontScale(1.0f); // Kembalikan ukuran font
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ma_engine_uninit(&engine);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard && currentCamState == STATE_FREE) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || currentCamState != STATE_FREE) return; 
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
        if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
        camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
        lastX = xpos; lastY = ypos;
    } else { firstMouse = true; }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 4);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture gagal di path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}