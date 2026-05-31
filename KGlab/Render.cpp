#include "Render.h"
#include "GUItextRectangle.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <math.h>
#include <vector>
#include <cctype>

#ifdef _DEBUG
#include <Debugapi.h>
struct debug_print
{
    template <class C> debug_print& operator<<(const C& a)
    {
        OutputDebugStringA((std::stringstream() << a).str().c_str());
        return *this;
    }
} debout;
#else
struct debug_print
{
    template <class C> debug_print& operator<<(const C& a)
    {
        return *this;
    }
} debout;
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

bool animateSea = false;
float seaTime = 0.0f;
bool texturing = true;
bool lightning = true;
bool umbrellasOpen = true;
float umbrellaAnim = 1.0f;   // 0 = закрыт, 1 = открыт
float umbrellaTarget = 1.0f; // куда стремимся
bool alpha = false;

// Глобальные данные для рельефа пляжа (статичные, вычисляются один раз)
std::vector<std::vector<float>> sandHeights;
float sandXMin = -200.0f, sandXMax = 200.0f;   // расширенный диапазон
float sandYMin = -200.0f, sandYMax = 100.0f;
float sandStep = 1.5f;
int sandNx = 0, sandNy = 0;

// Текстура песка и текстура неба
GLuint texId;
GLuint skyboxTexId = 0;
GLuint skyboxTex[6] = { 0 };


struct Umbrella
{
    float x;
    float y;
    float z;
    float radius;
    float height;
};

std::vector<Umbrella> umbrellas;

enum
{
    SKY_FRONT = 0,
    SKY_BACK,
    SKY_LEFT,
    SKY_RIGHT,
    SKY_TOP,
    SKY_BOTTOM
};

// Переключение режимов
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    char key = tolower(LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR)));
    switch (key)
    {
    case 'l': lightning = !lightning; break;
    case 't': texturing = !texturing; break;
    case 'a': alpha = !alpha; break;
    case 'm': animateSea = !animateSea; break; 
    case 'c':
        umbrellasOpen = !umbrellasOpen;
        umbrellaTarget = umbrellasOpen ? 1.0f : 0.0f;
        break;
    }
}

GuiTextRectangle text;

// ================= ФУНКЦИИ РИСОВАНИЯ =================
void DrawSand() {
    if (sandHeights.empty()) return;

    glColor3f(0.5f, 0.5f, 0.5f);   // белый цвет – текстура будет видна полностью

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < sandNx - 1; ++i) {
        float x0 = sandXMin + i * sandStep;
        float x1 = sandXMin + (i + 1) * sandStep;
        for (int j = 0; j < sandNy - 1; ++j) {
            float y0 = sandYMin + j * sandStep;
            float y1 = sandYMin + (j + 1) * sandStep;
            float z00 = sandHeights[i][j];
            float z10 = sandHeights[i + 1][j];
            float z01 = sandHeights[i][j + 1];
            float z11 = sandHeights[i + 1][j + 1];

            // текстурные координаты (масштаб 0.1)
            glTexCoord2f(x0 * 0.1f, y0 * 0.1f); glVertex3f(x0, y0, z00);
            glTexCoord2f(x1 * 0.1f, y0 * 0.1f); glVertex3f(x1, y0, z10);
            glTexCoord2f(x0 * 0.1f, y1 * 0.1f); glVertex3f(x0, y1, z01);

            glTexCoord2f(x1 * 0.1f, y1 * 0.1f); glVertex3f(x1, y1, z11);
            glTexCoord2f(x0 * 0.1f, y1 * 0.1f); glVertex3f(x0, y1, z01);
            glTexCoord2f(x1 * 0.1f, y0 * 0.1f); glVertex3f(x1, y0, z10);
        }
    }
    glEnd();
}

void DrawSea() {
    // Море покрывает только область X от -20 до 10 (дно + наклонный пляж)
    float seaXMin = -200.0f;
    float seaXMax = 100.0f;
    float seaYMin = sandYMin;
    float seaYMax = sandYMax;
    float seaStep = 1.0f;
    float baseZ = -0.05f;   // уровень воды (чуть выше дна)

    int nx = (int)((seaXMax - seaXMin) / seaStep) + 1;
    int ny = (int)((seaYMax - seaYMin) / seaStep) + 1;

    if (alpha) {
        glColor4f(0.2f, 0.5f, 0.8f, 0.7f);
    }
    else {
        glColor3f(0.2f, 0.5f, 0.8f);
    }

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < nx - 1; ++i) {
        float x0 = seaXMin + i * seaStep;
        float x1 = seaXMin + (i + 1) * seaStep;
        for (int j = 0; j < ny - 1; ++j) {
            float y0 = seaYMin + j * seaStep;
            float y1 = seaYMin + (j + 1) * seaStep;

            float z00 = baseZ, z10 = baseZ, z01 = baseZ, z11 = baseZ;
            if (animateSea) {
                float freq = 0.4f;
                float amp = 0.15f;
                z00 += amp * sinf(x0 * freq + seaTime) * cosf(y0 * freq);
                z10 += amp * sinf(x1 * freq + seaTime) * cosf(y0 * freq);
                z01 += amp * sinf(x0 * freq + seaTime) * cosf(y1 * freq);
                z11 += amp * sinf(x1 * freq + seaTime) * cosf(y1 * freq);
            }

            glVertex3f(x0, y0, z00);
            glVertex3f(x1, y0, z10);
            glVertex3f(x0, y1, z01);

            glVertex3f(x1, y1, z11);
            glVertex3f(x0, y1, z01);
            glVertex3f(x1, y0, z10);
        }
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

void DrawUmbrella()
{
    const int poleSegments = 12;
    const int roofSegments = 12;

    for (const auto& u : umbrellas)
    {
        //
        // Ножка
        //
        float poleRadius = 0.06f;

        glColor3f(0.45f, 0.25f, 0.10f);

        for (int i = 0; i < poleSegments; i++)
        {
            float a1 = 2.0f * 3.1415926f * i / poleSegments;
            float a2 = 2.0f * 3.1415926f * (i + 1) / poleSegments;

            float x1 = cosf(a1) * poleRadius;
            float y1 = sinf(a1) * poleRadius;

            float x2 = cosf(a2) * poleRadius;
            float y2 = sinf(a2) * poleRadius;

            glBegin(GL_QUADS);

            glNormal3f(cosf(a1), sinf(a1), 0);

            glVertex3f(u.x + x1, u.y + y1, u.z);
            glVertex3f(u.x + x2, u.y + y2, u.z);
            glVertex3f(u.x + x2, u.y + y2, u.z + u.height);
            glVertex3f(u.x + x1, u.y + y1, u.z + u.height);

            glEnd();
        }

        //
        // Крыша
        //
        float roofZ = u.z + u.height;
        float openFactor = umbrellasOpen ? 1.0f : 0.2f;

        float anim = umbrellaAnim;

        // радиус "раскрытия"
        float radius = u.radius * (0.3f + 0.7f * anim);

        // высота купола
        float roofTop = roofZ + 0.1f + 0.5f * anim;


        glColor3f(
            0.8f,
            0.15f + (u.x * 0.03f),
            0.1f + fabs(sinf(u.y))
        );

        glBegin(GL_TRIANGLES);

        for (int i = 0; i < roofSegments; i++)
        {
            float a1 = 2.0f * 3.1415926f * i / roofSegments;
            float a2 = 2.0f * 3.1415926f * (i + 1) / roofSegments;

            float x1 = u.x + cosf(a1) * radius;
            float y1 = u.y + sinf(a1) * radius;

            float x2 = u.x + cosf(a2) * radius;
            float y2 = u.y + sinf(a2) * radius;

            glNormal3f(0, 0, 1);

            glVertex3f(u.x, u.y, roofTop);
            glVertex3f(x1, y1, roofZ);
            glVertex3f(x2, y2, roofZ);
        }

        glEnd();
    }
}

// ================= SKYBOX (крест 4x3) =================
// ================= SKYBOX (крест 4x3) =================
void loadSkyboxTexture(const char* path) {
    int width, height, n;
    unsigned char* data = stbi_load(path, &width, &height, &n, 4);
    if (!data) {
        debout << "Failed to load skybox texture: " << path << "\n";
        return;
    }

    debout << "Loaded " << path << ": " << width << "x" << height << ", channels " << n << "\n";

    // Проверяем, что это крест 4x3
    if (width % 4 != 0 || height % 3 != 0) {
        debout << "Skybox texture is not a 4x3 cross layout (width%4=" << width % 4
            << ", height%3=" << height % 3 << ")\n";
        stbi_image_free(data);
        return;
    }

    int sqW = width / 4;
    int sqH = height / 3;
    debout << "Square size: " << sqW << "x" << sqH << "\n";

    // Создаём текстуру
    glGenTextures(1, &skyboxTexId);
    glBindTexture(GL_TEXTURE_2D, skyboxTexId);

    // Переворачиваем по Y (stbi загружает сверху вниз)
    unsigned char* flipped = new unsigned char[width * height * 4];
    for (int row = 0; row < height; ++row) {
        memcpy(flipped + row * width * 4,
            data + (height - 1 - row) * width * 4,
            width * 4);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, flipped);
    delete[] flipped;
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    debout << "Skybox texture loaded successfully, ID=" << skyboxTexId << "\n";
    MessageBoxA(NULL,
        skyboxTexId ? "skybox loaded" : "skybox failed",
        "debug",
        MB_OK);
}

void DrawSkybox()
{
    float s = 80.0f;

    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);

    glDepthMask(GL_FALSE);

    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);

    //
    // FRONT
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_FRONT]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glEnd();

    //
    // BACK
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_BACK]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    //
    // LEFT
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_LEFT]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    //
    // RIGHT
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_RIGHT]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    //
    // TOP
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_TOP]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    //
    // BOTTOM
    //
    glBindTexture(GL_TEXTURE_2D, skyboxTex[SKY_BOTTOM]);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, -s);
    glEnd();

    glDepthMask(GL_TRUE);

    glPopAttrib();
}

GLuint LoadTexture(const char* fileName)
{
    int w, h, n;

    unsigned char* data = stbi_load(fileName, &w, &h, &n, 4);

    if (!data)
    {
        std::cout << "Cannot load " << fileName << std::endl;
        return 0;
    }

    for (int y = 0; y < h / 2; y++)
    {
        for (int x = 0; x < w * 4; x++)
        {
            std::swap(
                data[y * w * 4 + x],
                data[(h - 1 - y) * w * 4 + x]
            );
        }
    }

    GLuint tex;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        w,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    stbi_image_free(data);

    return tex;
}

// ===================================================

void initRender()
{
    // Настройка текстур песка
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    int x, y, n;
    unsigned char* data = stbi_load("texture.png", &x, &y, &n, 4);
    if (data) {
        unsigned char* _tmp = new unsigned char[x * 4];
        for (int i = 0; i < y / 2; ++i) {
            std::memcpy(_tmp, data + i * x * 4, x * 4);
            std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4);
            std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4);
        }
        delete[] _tmp;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Генерация рельефа песка с тремя зонами:
    // - X = -20..-10 : дно (горизонтально, Z = -1.2)
    // - X = -10..10  : наклонный пляж (Z от -1.2 до 0.6)
    // - X = 10..20   : суша (горизонтально, Z = 0.6)
    float bottomHeight = -1.2f;
    float shoreHeight = 0.6f;
    float tiltStartX = -10.0f;
    float tiltEndX = 10.0f;

    sandNx = (int)((sandXMax - sandXMin) / sandStep) + 1;
    sandNy = (int)((sandYMax - sandYMin) / sandStep) + 1;
    sandHeights.assign(sandNx, std::vector<float>(sandNy, 0.0f));

    for (int i = 0; i < sandNx; ++i) {
        float x = sandXMin + i * sandStep;
        float baseZ;
        if (x < tiltStartX) {
            baseZ = bottomHeight;
        }
        else if (x > tiltEndX) {
            baseZ = shoreHeight;
        }
        else {
            // линейная интерполяция от bottomHeight до shoreHeight
            float t = (x - tiltStartX) / (tiltEndX - tiltStartX);
            baseZ = bottomHeight * (1 - t) + shoreHeight * t;
        }

        for (int j = 0; j < sandNy; ++j) {
            float y = sandYMin + j * sandStep;
            // мелкие неровности
            float noise = 0.1f * sinf(x * 0.8f) * cosf(y * 0.5f)
                + 0.05f * sinf(y * 1.2f + 1.0f);
            float z = baseZ + noise;
            // ограничения
            if (z > 0.8f) z = 0.8f;
            if (z < -1.4f) z = -1.4f;
            sandHeights[i][j] = z;
        }
    }

    srand(12345);

    const int umbrellaCount = 60;
    const float minDistance = 4.0f;

    for (int i = 0; i < umbrellaCount;)
    {
        Umbrella u;

        u.x = 12.0f + (rand() % 10000) / 10000.0f *50.0f;
        u.y = -100.0f + (rand() % 10000) / 10000.0f * 200.0f;

        bool tooClose = false;

        for (const auto& other : umbrellas)
        {
            float dx = u.x - other.x;
            float dy = u.y - other.y;

            if (dx * dx + dy * dy < minDistance * minDistance)
            {
                tooClose = true;
                break;
            }
        }

        if (tooClose)
            continue;

        int ix = (int)((u.x - sandXMin) / sandStep);
        int iy = (int)((u.y - sandYMin) / sandStep);

        if (ix < 0)
            ix = 0;
        if (ix >= sandNx)
            ix = sandNx - 1;

        if (iy < 0)
            iy = 0;
        if (iy >= sandNy)
            iy = sandNy - 1;

        u.z = sandHeights[ix][iy];

        u.radius = 0.8f + (rand() % 1000) / 1000.0f * 0.6f;
        u.height = 1.8f + (rand() % 1000) / 1000.0f * 0.5f;

        umbrellas.push_back(u);

        ++i;
    }

    // Настройка камеры
    camera.caclulateCameraPos();
    camera.setPosition(0.0, -12.0, 18.0);

    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);

    // Настройка света
    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);

    gl.KeyDownEvent.reaction(switchModes);
    text.setSize(512, 180);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    skyboxTex[SKY_FRONT] = LoadTexture("top.png");
    skyboxTex[SKY_BACK] = LoadTexture("bottom.png");
    skyboxTex[SKY_LEFT] = LoadTexture("right.png");
    skyboxTex[SKY_RIGHT] = LoadTexture("left.png");
    skyboxTex[SKY_TOP] = LoadTexture("front.png");
    skyboxTex[SKY_BOTTOM] = LoadTexture("back.png");
}

void Render(double delta_time)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    if (gl.isKeyPressed('F'))
        light.SetPosition(camera.x(), camera.y(), camera.z());

    if (animateSea) {
        seaTime += delta_time * 2.0f;
    }

    camera.SetUpCamera();
    light.SetUpLight();

    // Рисуем skybox (он должен быть позади всей сцены, используется отдельная текстура)
    DrawSkybox();

    gl.DrawAxes();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    if (lightning) glEnable(GL_LIGHTING);
    if (texturing) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
    }
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    float amb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float spec[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float sh = 0.2f * 256.0f;
    glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, sh);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);


    float speed = 2.5f; // скорость раскрытия

    if (umbrellaAnim < umbrellaTarget)
    {
        umbrellaAnim += speed * delta_time;
        if (umbrellaAnim > umbrellaTarget)
            umbrellaAnim = umbrellaTarget;
    }
    else if (umbrellaAnim > umbrellaTarget)
    {
        umbrellaAnim -= speed * delta_time;
        if (umbrellaAnim < umbrellaTarget)
            umbrellaAnim = umbrellaTarget;
    }

    // ============= РИСУЕМ СЦЕНУ =============
    DrawSand();
    DrawSea();
    DrawUmbrella();
    // ========================================

    light.DrawLightGizmo();

    // 2D текст
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3)
        << "T - " << (texturing ? L"[вкл]выкл" : L"вкл[выкл]") << L" текстур\n"
        << "L - " << (lightning ? L"[вкл]выкл" : L"вкл[выкл]") << L" освещение\n"
        << "A - " << (alpha ? L"[вкл]выкл" : L"вкл[выкл]") << L" альфа-наложение\n"
        << L"M - " << (animateSea ? L"[вкл]выкл" : L"вкл[выкл]") << L" анимация волн\n"
        << L"F - свет в позицию камеры\n"
        << L"G - двигать свет по горизонтали\n"
        << L"G+ЛКМ - двигать свет по вертикали\n"
        << L"Свет: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")\n"
        << L"Камера: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")\n"
        << L"R=" << std::setw(7) << camera.distance() << ", fi1=" << std::setw(7) << camera.fi1()
        << ", fi2=" << std::setw(7) << camera.fi2() << '\n'
        << L"delta_time: " << std::setprecision(5) << delta_time << std::endl;

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}