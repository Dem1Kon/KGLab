#include "Render.h"
#include "GUItextRectangle.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <math.h>

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

// Библиотека для разгрузки изображений
// https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

double A[] = { 1, 0, 0 };
double B[] = { 4, 5, 0 };
double C[] = { 0, 2, 0 };
double D[] = { -5, 5, 0 };
double E[] = { -8, 0, 0 };
double F[] = { -5, -5, 0 };
double G[] = { 0, -1, 0 };
double H[] = { 4, -3, 0 };

double A1[] = { 1, 0, 4 };
double B1[] = { 4, 5, 4 };
double C1[] = { 0, 2, 4 };
double D1[] = { -5, 5, 4 };
double E1[] = { -8, 0, 4 };
double F1[] = { -5, -5, 4 };
double G1[] = { 0, -1, 4 };
double H1[] = { 4, -3, 4 };


bool texturing = true;
bool lightning = true;
bool alpha = false;

double dx = 5 - (-1);
double dy = -5 - (-8);
double len = sqrt(dx * dx + dy * dy);

double cx = (-1 + 5) * 0.5;
double cy = (-8 + -5) * 0.5;

double dirx = dx / len;
double diry = dy / len;

double perpx = -diry;
double perpy = dirx;

double R = len * 0.5;
double nx = -dy / len;
double ny = dx / len;


double* setN(double* A, double* B, double* C)
{
    static double N[3];

    double u[3] = { B[0] - A[0], B[1] - A[1], B[2] - A[2] };
    double v[3] = { C[0] - A[0], C[1] - A[1], C[2] - A[2] };

    N[0] = u[1] * v[2] - u[2] * v[1];
    N[1] = u[2] * v[0] - u[0] * v[2];
    N[2] = u[0] * v[1] - u[1] * v[0];

    double l = sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);
    if (l != 0) {
        N[0] /= l;
        N[1] /= l;
        N[2] /= l;
    }

    return N;
}

void shapeTop() {
    // Вычисляем bounding box вершин основания (можно вынести в initRender для оптимизации)
    double minX = A[0], maxX = A[0], minY = A[1], maxY = A[1];
    double* verts[] = { A, B, C, D, E, F, G, H };
    for (int i = 0; i < 8; ++i) {
        if (verts[i][0] < minX) minX = verts[i][0];
        if (verts[i][0] > maxX) maxX = verts[i][0];
        if (verts[i][1] < minY) minY = verts[i][1];
        if (verts[i][1] > maxY) maxY = verts[i][1];
    }
    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    if (rangeX == 0) rangeX = 1;
    if (rangeY == 0) rangeY = 1;

    glBegin(GL_TRIANGLES);
    glNormal3d(0, 0, -1);

    // Треугольник A-B-C
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((B[0] - minX) / rangeX, (B[1] - minY) / rangeY); glVertex3dv(B);
    glTexCoord2d((C[0] - minX) / rangeX, (C[1] - minY) / rangeY); glVertex3dv(C);

    // A-C-D
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((C[0] - minX) / rangeX, (C[1] - minY) / rangeY); glVertex3dv(C);
    glTexCoord2d((D[0] - minX) / rangeX, (D[1] - minY) / rangeY); glVertex3dv(D);

    // A-D-E
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((D[0] - minX) / rangeX, (D[1] - minY) / rangeY); glVertex3dv(D);
    glTexCoord2d((E[0] - minX) / rangeX, (E[1] - minY) / rangeY); glVertex3dv(E);

    // A-E-F
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((E[0] - minX) / rangeX, (E[1] - minY) / rangeY); glVertex3dv(E);
    glTexCoord2d((F[0] - minX) / rangeX, (F[1] - minY) / rangeY); glVertex3dv(F);

    // A-F-G
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((F[0] - minX) / rangeX, (F[1] - minY) / rangeY); glVertex3dv(F);
    glTexCoord2d((G[0] - minX) / rangeX, (G[1] - minY) / rangeY); glVertex3dv(G);

    // A-G-H
    glTexCoord2d((A[0] - minX) / rangeX, (A[1] - minY) / rangeY); glVertex3dv(A);
    glTexCoord2d((G[0] - minX) / rangeX, (G[1] - minY) / rangeY); glVertex3dv(G);
    glTexCoord2d((H[0] - minX) / rangeX, (H[1] - minY) / rangeY); glVertex3dv(H);

    glEnd();
}

void shapeBot() {
    double minX = A1[0], maxX = A1[0], minY = A1[1], maxY = A1[1];
    double* verts[] = { A1, B1, C1, D1, E1, F1, G1, H1 };
    for (int i = 0; i < 8; ++i) {
        if (verts[i][0] < minX) minX = verts[i][0];
        if (verts[i][0] > maxX) maxX = verts[i][0];
        if (verts[i][1] < minY) minY = verts[i][1];
        if (verts[i][1] > maxY) maxY = verts[i][1];
    }
    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    if (rangeX == 0) rangeX = 1;
    if (rangeY == 0) rangeY = 1;

    glBegin(GL_TRIANGLES);
    glNormal3d(0, 0, 1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((B1[0] - minX) / rangeX, (B1[1] - minY) / rangeY); glVertex3dv(B1);
    glTexCoord2d((C1[0] - minX) / rangeX, (C1[1] - minY) / rangeY); glVertex3dv(C1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((C1[0] - minX) / rangeX, (C1[1] - minY) / rangeY); glVertex3dv(C1);
    glTexCoord2d((D1[0] - minX) / rangeX, (D1[1] - minY) / rangeY); glVertex3dv(D1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((D1[0] - minX) / rangeX, (D1[1] - minY) / rangeY); glVertex3dv(D1);
    glTexCoord2d((E1[0] - minX) / rangeX, (E1[1] - minY) / rangeY); glVertex3dv(E1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((E1[0] - minX) / rangeX, (E1[1] - minY) / rangeY); glVertex3dv(E1);
    glTexCoord2d((F1[0] - minX) / rangeX, (F1[1] - minY) / rangeY); glVertex3dv(F1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((F1[0] - minX) / rangeX, (F1[1] - minY) / rangeY); glVertex3dv(F1);
    glTexCoord2d((G1[0] - minX) / rangeX, (G1[1] - minY) / rangeY); glVertex3dv(G1);

    glTexCoord2d((A1[0] - minX) / rangeX, (A1[1] - minY) / rangeY); glVertex3dv(A1);
    glTexCoord2d((G1[0] - minX) / rangeX, (G1[1] - minY) / rangeY); glVertex3dv(G1);
    glTexCoord2d((H1[0] - minX) / rangeX, (H1[1] - minY) / rangeY); glVertex3dv(H1);

    glEnd();
}

void shapeSides() {
    glBegin(GL_QUADS);

    // Грань A-B
    auto N1 = setN(A1, A, B1);
    glNormal3dv(N1);
    glTexCoord2d(0, 0); glVertex3dv(A);
    glTexCoord2d(1, 0); glVertex3dv(B);
    glTexCoord2d(1, 1); glVertex3dv(B1);
    glTexCoord2d(0, 1); glVertex3dv(A1);

    // Грань B-C
    auto N2 = setN(B1, B, C1);
    glNormal3dv(N2);
    glTexCoord2d(0, 0); glVertex3dv(B);
    glTexCoord2d(1, 0); glVertex3dv(C);
    glTexCoord2d(1, 1); glVertex3dv(C1);
    glTexCoord2d(0, 1); glVertex3dv(B1);

    // Грань C-D
    auto N3 = setN(C1, C, D1);
    glNormal3dv(N3);
    glTexCoord2d(0, 0); glVertex3dv(C);
    glTexCoord2d(1, 0); glVertex3dv(D);
    glTexCoord2d(1, 1); glVertex3dv(D1);
    glTexCoord2d(0, 1); glVertex3dv(C1);

    // Грань D-E
    auto N4 = setN(D1, D, E1);
    glNormal3dv(N4);
    glTexCoord2d(0, 0); glVertex3dv(D);
    glTexCoord2d(1, 0); glVertex3dv(E);
    glTexCoord2d(1, 1); glVertex3dv(E1);
    glTexCoord2d(0, 1); glVertex3dv(D1);

    // Грань E-F
    auto N5 = setN(E1, E, F1);
    glNormal3dv(N5);
    glTexCoord2d(0, 0); glVertex3dv(E);
    glTexCoord2d(1, 0); glVertex3dv(F);
    glTexCoord2d(1, 1); glVertex3dv(F1);
    glTexCoord2d(0, 1); glVertex3dv(E1);

    // Грань F-G
    auto N6 = setN(F1, F, G1);
    glNormal3dv(N6);
    glTexCoord2d(0, 0); glVertex3dv(F);
    glTexCoord2d(1, 0); glVertex3dv(G);
    glTexCoord2d(1, 1); glVertex3dv(G1);
    glTexCoord2d(0, 1); glVertex3dv(F1);

    // Грань G-H
    auto N7 = setN(G1, G, H1);
    glNormal3dv(N7);
    glTexCoord2d(0, 0); glVertex3dv(G);
    glTexCoord2d(1, 0); glVertex3dv(H);
    glTexCoord2d(1, 1); glVertex3dv(H1);
    glTexCoord2d(0, 1); glVertex3dv(G1);

    // Грань H-A
    auto N8 = setN(H1, H, A1);
    glNormal3dv(N8);
    glTexCoord2d(0, 0); glVertex3dv(H);
    glTexCoord2d(1, 0); glVertex3dv(A);
    glTexCoord2d(1, 1); glVertex3dv(A1);
    glTexCoord2d(0, 1); glVertex3dv(H1);

    glEnd();
}

void shape() {
    shapeTop();
    shapeBot();
    shapeSides();
}

// Переключение режимов освещения, текстурирования, альфа-наложения
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    // Конвертируем код клавиши в букву
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

    switch (key)
    {
    case 'L':
        lightning = !lightning;
        break;
    case 'T':
        texturing = !texturing;
        break;
    case 'A':
        alpha = !alpha;
        break;
    }
}

// Текстовый прямоугольник в верхнем правом углу.
// OGL не предоставляет возможности для хранения текста;
// внутри этого класса создается картинка с текстом (через GDI),
// в виде текстуры накладывается на прямоугольник и рисуется на экране.
// Это самый простой, но очень неэффективный способ написать что-либо на экране.
GuiTextRectangle text;

// ID для текстуры
GLuint texId;

// Выполняется один раз перед первым рендером
void initRender()
{
    //==============НАСТРОЙКА ТЕКСТУР================
    // 4 байта на хранение пикселя
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // Генерируем ID текстуры
    glGenTextures(1, &texId);

    // Делаем текущую текстуру активной
    glBindTexture(GL_TEXTURE_2D, texId);

    int x, y, n;

    // Загружаем картинку
    // см. #include "stb_image.h"
    unsigned char* data = stbi_load("texture.png", &x, &y, &n, 4);
    // x - ширина изображения
    // y - высота изображения
    // n - количество каналов
    // 4 - нужное нам количество каналов
    // Пиксели будут хранится в памяти [R-G-B-A]-[R-G-B-A]-[.....
    //  по 4 байта на пиксель - по байту на канал
    // Пустые каналы будут равны 255

    // Картинка хранится в памяти перевернутой
    // так как ее начало в левом верхнем углу;
    // по этому мы ее переворачиваем -
    // меняем первую строку с последней,
    // вторую с предпоследней, и.т.д.
    unsigned char* _tmp = new unsigned char[x * 4];
    for (int i = 0; i < y / 2; ++i)
    {
        std::memcpy(_tmp, data + i * x * 4, x * 4);                       // переносим строку i в tmp
        std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4); // (y-1-i)я строка -> iя строка
        std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4);             // tmp -> (y-1-i)я строка
    }
    delete[] _tmp;

    // Загрузка изображения в видеопамять
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Выгрузка изображения из оперативной памяти
    stbi_image_free(data);

    // Настройка режима наложения текстур
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    // GL_REPLACE -- полная замена политога текстурой
    // Настройка тайлинга
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Настройка фильтрации
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //======================================================

    //================НАСТРОЙКА КАМЕРЫ======================
    camera.caclulateCameraPos();

    // Привязываем камеру к событиям "движка"
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
    //==============НАСТРОЙКА СВЕТА===========================
    // Привязываем свет к событиям "движка"
    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
    //========================================================
    //====================Прочее==============================
    gl.KeyDownEvent.reaction(switchModes);
    text.setSize(512, 180);
    //========================================================

    camera.setPosition(2, 1.5, 1.5);
}

void Render(double delta_time)
{
    glEnable(GL_DEPTH_TEST);

    // Настройка камеры и света
    if (gl.isKeyPressed('F')) // если нажата F - свет из камеры
    {
        light.SetPosition(camera.x(), camera.y(), camera.z());
    }
    camera.SetUpCamera();
    light.SetUpLight();

    // Рисуем оси
    gl.DrawAxes();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Переключаем режимы (см void switchModes(OpenGL *sender, KeyEventArg arg))
    if (lightning)
        glEnable(GL_LIGHTING);
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId); // Сбрасываем текущую текстуру
    }

    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    //=============НАСТРОЙКА МАТЕРИАЛА==============

    // Настройка материала, все что рисуется ниже будет иметь этот материал.
    // Массивы с настройками материала
    float amb[] = { 0, 0, 0, 0 };
    float dif[] = { 1, 1, 1, 1 };
    float spec[] = { 1, 1, 1, 1 };
    float sh = 0.1f * 256;

    // Фоновая
    glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    // Дифузная
    glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
    // Зеркальная
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    // Размер блика
    glMaterialf(GL_FRONT, GL_SHININESS, sh);

    // Сглаживание освещения
    glShadeModel(GL_SMOOTH); // закраска по Гуро
    //(GL_SMOOTH - плоская закраска)

//============ РИСОВАТЬ ТУТ ==============

    shape();
    //===============================================

    // Рисуем источник света
    light.DrawLightGizmo();

    //================Сообщение в верхнем левом углу=======================

    // Переключаемся на матрицу проекции
    glMatrixMode(GL_PROJECTION);
    // Сохраняем текущую матрицу проекции с перспективным преобразованием
    glPushMatrix();
    // Загружаем единичную матрицу в матрицу проекции
    glLoadIdentity();

    // Устанавливаем матрицу параллельной проекции
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

    // Переключаемся на матрицу MODELVIEW
    glMatrixMode(GL_MODELVIEW);
    // Сохраняем матрицу
    glPushMatrix();
    // Сбрасываем все трансформации и настройки камеры загрузкой единичной матрицы
    glLoadIdentity();

    // Нарисованное тут находится в 2D системе координат
    // Нижний левый угол окна - точка (0,0)
    // Верхний правый угол (ширина_окна - 1, высота_окна - 1)

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3) << "T - " << (texturing ? L"[вкл]выкл" : L"вкл[выкл]") << L" текстур\n"
        << "L - " << (lightning ? L"[вкл]выкл" : L"вкл[выкл]") << L" освещение\n"
        << "A - " << (alpha ? L"[вкл]выкл" : L"вкл[выкл]") << L" альфа-наложение\n"
        << L"F - переместить свет в позицию камеры\n"
        << L"G - двигать свет по горизонтали\n"
        << L"G+ЛКМ - двигать свет по вертикали\n"
        << L"Координаты света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7)
        << light.z() << ")\n"
        << L"Координаты камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << ","
        << std::setw(7) << camera.z() << ")\n"
        << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ", fi1=" << std::setw(7) << camera.fi1()
        << ", fi2=" << std::setw(7) << camera.fi2() << '\n'
        << L"delta_time: " << std::setprecision(5) << delta_time << std::endl;

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    // Восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void cylinder(double cx, double cy, double R, double h, int n)
{
    for (int i = 0; i < n; i++)
    {
        double t1 = 3.1415926 * i / n;
        double t2 = 3.1415926 * (i + 1) / n;

        double s1 = sin(t1);
        double c1 = cos(t1);
        double s2 = sin(t2);
        double c2 = cos(t2);

        double x1 = cx - R * (c1 * (dx / len) + s1 * nx);
        double y1 = cy - R * (c1 * (dy / len) + s1 * ny);

        double x2 = cx - R * (c2 * (dx / len) + s2 * nx);
        double y2 = cy - R * (c2 * (dy / len) + s2 * ny);


        glBegin(GL_QUADS);
        glColor3d(0.18, 0.18, 0.81);
        glVertex3d(x1, y1, 0);
        glVertex3d(x2, y2, 0);
        glVertex3d(x2, y2, h);
        glVertex3d(x1, y1, h);
        glEnd();
    }

    glBegin(GL_TRIANGLES);

    for (int i = 0; i < n; i++)
    {
        double t1 = 3.1415926 * i / n;
        double t2 = 3.1415926 * (i + 1) / n;

        double x0 = cx;
        double y0 = cy;
        double s1 = sin(t1);
        double c1 = cos(t1);
        double s2 = sin(t2);
        double c2 = cos(t2);

        double x1 = cx - R * (c1 * (dx / len) + s1 * nx);
        double y1 = cy - R * (c1 * (dy / len) + s1 * ny);

        double x2 = cx - R * (c2 * (dx / len) + s2 * nx);
        double y2 = cy - R * (c2 * (dy / len) + s2 * ny);

        glColor3d(1, 1, 0);
        glVertex3d(x0, y0, h);
        glVertex3d(x1, y1, h);
        glVertex3d(x2, y2, h);
    }

    glEnd();

    glBegin(GL_TRIANGLES);

    for (int i = 0; i < n; i++)
    {
        double t1 = 3.1415926 * i / n;
        double t2 = 3.1415926 * (i + 1) / n;

        double x0 = cx;
        double y0 = cy;
        double s1 = sin(t1);
        double c1 = cos(t1);
        double s2 = sin(t2);
        double c2 = cos(t2);

        double x1 = cx - R * (c1 * (dx / len) + s1 * nx);
        double y1 = cy - R * (c1 * (dy / len) + s1 * ny);

        double x2 = cx - R * (c2 * (dx / len) + s2 * nx);
        double y2 = cy - R * (c2 * (dy / len) + s2 * ny);

        glColor3d(0.8, 0.9, 0.9);
        glVertex3d(x0, y0, 0);
        glVertex3d(x1, y1, 0);
        glVertex3d(x2, y2, 0);
    }

    glEnd();

}