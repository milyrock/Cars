#include "Render.h"
#include "GUItextRectangle.h"
#include "Texture.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "debout.h"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

// ========================== СОСТОЯНИЕ СЦЕНЫ ==============================
bool texturing = true;
bool lightning = true;
bool alpha = true;

bool rampRequested = false;     // пользователь открыл пандус
bool carDeparture = false;      // автомобиль выезжает
bool headlightsOn = false;      // фары

float rampAngle = 0.0f;         // 0 - закрыт, 105 - лежит на земле
float carTravel = 0.0f;         // путь автомобиля наружу

double full_time = 0.0;

GuiTextRectangle text;
Texture trailerLogo;

// ============================ МАТЕРИАЛЫ ===================================
void SetMaterial(float r, float g, float b, float shininess = 35.0f,
                 float sr = 0.30f, float sg = 0.30f, float sb = 0.30f,
                 float alphaValue = 1.0f)
{
    GLfloat ambient[]  = { r * 0.25f, g * 0.25f, b * 0.25f, alphaValue };
    GLfloat diffuse[]  = { r, g, b, alphaValue };
    GLfloat specular[] = { sr, sg, sb, alphaValue };
    GLfloat emission[] = { 0.0f, 0.0f, 0.0f, alphaValue };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void SetEmissiveMaterial(float r, float g, float b)
{
    SetMaterial(r, g, b, 60.0f, 0.8f, 0.8f, 0.65f);
    GLfloat emission[] = { r * 0.85f, g * 0.85f, b * 0.70f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

// ========================== БАЗОВЫЕ ПРИМИТИВЫ =============================
void DrawUnitCube()
{
    glBegin(GL_QUADS);
    // Передняя грань (+Z)
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f,  0.5f);
    glTexCoord2f(1, 0); glVertex3f( 0.5f, -0.5f,  0.5f);
    glTexCoord2f(1, 1); glVertex3f( 0.5f,  0.5f,  0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f,  0.5f,  0.5f);
    // Задняя грань (-Z)
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f( 0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f(-0.5f,  0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f( 0.5f,  0.5f, -0.5f);
    // Левая грань (-X)
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f(-0.5f, -0.5f,  0.5f);
    glTexCoord2f(1, 1); glVertex3f(-0.5f,  0.5f,  0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f,  0.5f, -0.5f);
    // Правая грань (+X)
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(0.5f, -0.5f,  0.5f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f(0.5f,  0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(0.5f,  0.5f,  0.5f);
    // Верхняя грань (+Y)
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, 0.5f,  0.5f);
    glTexCoord2f(1, 0); glVertex3f( 0.5f, 0.5f,  0.5f);
    glTexCoord2f(1, 1); glVertex3f( 0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, -0.5f);
    // Нижняя грань (-Y)
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f( 0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f( 0.5f, -0.5f,  0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, -0.5f,  0.5f);
    glEnd();
}

void DrawBox(float x, float y, float z, float sx, float sy, float sz)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    DrawUnitCube();
    glPopMatrix();
}

void DrawWheel(float x, float y, float z, float radius, float width)
{
    GLUquadric* q = gluNewQuadric();
    SetMaterial(0.035f, 0.035f, 0.04f, 10.0f, 0.08f, 0.08f, 0.08f);
    glPushMatrix();
    glTranslatef(x, y, z - width * 0.5f);
    gluCylinder(q, radius, radius, width, 24, 1);
    gluDisk(q, 0.0, radius, 24, 1);
    glTranslatef(0, 0, width);
    gluDisk(q, 0.0, radius, 24, 1);
    glPopMatrix();

    SetMaterial(0.62f, 0.62f, 0.66f, 70.0f, 0.75f, 0.75f, 0.75f);
    glPushMatrix();
    glTranslatef(x, y, z - width * 0.5f - 0.002f);
    gluDisk(q, 0.0, radius * 0.48f, 18, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(x, y, z + width * 0.5f + 0.002f);
    gluDisk(q, 0.0, radius * 0.48f, 18, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void DrawSphere(float x, float y, float z, float r)
{
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(x, y, z);
    gluSphere(q, r, 20, 12);
    glPopMatrix();
    gluDeleteQuadric(q);
}

// ============================= ДОРОГА =====================================
void DrawGround()
{
    glDisable(GL_TEXTURE_2D);
    SetMaterial(0.15f, 0.17f, 0.19f, 8.0f, 0.06f, 0.06f, 0.06f);
    DrawBox(0.7f, -0.08f, 0.0f, 13.0f, 0.15f, 6.0f);

    // Разметка дороги
    SetMaterial(0.95f, 0.80f, 0.12f, 5.0f, 0.10f, 0.10f, 0.10f);
    for (int i = -3; i <= 3; ++i)
        DrawBox((float)i * 1.55f, 0.005f, 2.15f, 0.85f, 0.012f, 0.065f);
}

// ============================== ТРЕЙЛЕР ===================================
void DrawTrailerLogo(float trailerCenter, float trailerLength)
{
    SetMaterial(0.92f, 0.92f, 0.94f, 30.0f, 0.25f, 0.25f, 0.25f);
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        trailerLogo.Bind();
    }
    else
        glDisable(GL_TEXTURE_2D);

    const float left  = trailerCenter - trailerLength * 0.45f;
    const float right = trailerCenter + trailerLength * 0.45f;
    const float low   = 0.72f;
    const float high  = 2.00f;

    // Логотип на ближней боковой стенке (+Z).
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(left,  low,   1.066f);
    glTexCoord2f(1, 0); glVertex3f(right, low,   1.066f);
    glTexCoord2f(1, 1); glVertex3f(right, high,  1.066f);
    glTexCoord2f(0, 1); glVertex3f(left,  high,  1.066f);
    glEnd();

    // Та же текстура на противоположной стороне (-Z), без зеркальной надписи снаружи.
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(right, low,  -1.066f);
    glTexCoord2f(1, 0); glVertex3f(left,  low,  -1.066f);
    glTexCoord2f(1, 1); glVertex3f(left,  high, -1.066f);
    glTexCoord2f(0, 1); glVertex3f(right, high, -1.066f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}
void DrawRamp(float rearX)
{
    glDisable(GL_TEXTURE_2D);
    SetMaterial(0.32f, 0.34f, 0.37f, 45.0f, 0.48f, 0.48f, 0.48f);
    glPushMatrix();
    glTranslatef(rearX, 0.53f, 0.0f); // нижний шарнир задней двери
    glRotatef(rampAngle, 0, 0, 1);
    DrawBox(0.0f, 0.78f, 0.0f, 0.09f, 1.56f, 1.92f);
    SetMaterial(0.66f, 0.67f, 0.69f, 70.0f, 0.7f, 0.7f, 0.7f);
    for (int i = -2; i <= 2; ++i)
        DrawBox(-0.052f, 0.78f, i * 0.32f, 0.015f, 1.42f, 0.035f);
    glPopMatrix();
}

void DrawTruckWindows()
{
    // Стёкла тягача теперь утоплены в кабину и имеют рамы.
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    SetMaterial(0.12f, 0.38f, 0.58f, 70.0f, 0.50f, 0.58f, 0.65f, alpha ? 0.63f : 1.0f);
    // Боковые окна с обеих сторон — слегка утоплены в корпус кабины.
    DrawBox(4.95f, 1.45f, -0.845f, 0.30f, 0.24f, 0.010f);
    DrawBox(4.95f, 1.45f,  0.845f, 0.30f, 0.24f, 0.010f);
    DrawBox(5.17f, 1.45f, -0.845f, 0.12f, 0.24f, 0.010f);
    DrawBox(5.17f, 1.45f,  0.845f, 0.12f, 0.24f, 0.010f);
    // Лобовое стекло расположено на передней наружной поверхности кабины.
    // Оно чуть вынесено вперёд, чтобы не скрывалось корпусом.
    DrawBox(5.466f, 1.43f, 0.0f, 0.012f, 0.29f, 1.22f);

    if (alpha)
    {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void DrawTrailer()
{
    // Прицеп удлинён назад: машина полностью находится внутри до старта.
    const float trailerFront = 4.15f;
    const float trailerRear  = -0.75f;
    const float trailerLength = trailerFront - trailerRear;
    const float tx = (trailerFront + trailerRear) * 0.5f;
    glDisable(GL_TEXTURE_2D);

    SetMaterial(0.70f, 0.72f, 0.75f, 58.0f, 0.55f, 0.55f, 0.55f);
    DrawBox(tx, 0.54f, 0.0f, trailerLength, 0.13f, 2.12f);
    DrawBox(tx, 2.12f, 0.0f, trailerLength, 0.13f, 2.12f);
    DrawBox(tx, 1.34f, -1.02f, trailerLength, 1.48f, 0.08f);
    DrawBox(tx, 1.34f,  1.02f, trailerLength, 1.48f, 0.08f);
    DrawBox(trailerFront, 1.34f, 0.0f, 0.10f, 1.60f, 2.12f);

    // Кабина тягача.
    SetMaterial(0.16f, 0.18f, 0.22f, 50.0f, 0.55f, 0.55f, 0.55f);
    DrawBox(4.82f, 0.85f, 0.0f, 1.28f, 0.72f, 1.85f);
    DrawBox(5.10f, 1.42f, 0.0f, 0.72f, 0.52f, 1.74f);
    DrawTruckWindows();

    DrawTrailerLogo(tx, trailerLength);
    DrawRamp(trailerRear);

    DrawWheel(0.55f, 0.36f, -1.13f, 0.34f, 0.15f);
    DrawWheel(0.55f, 0.36f,  1.13f, 0.34f, 0.15f);
    DrawWheel(2.35f, 0.36f, -1.13f, 0.34f, 0.15f);
    DrawWheel(2.35f, 0.36f,  1.13f, 0.34f, 0.15f);
    DrawWheel(3.35f, 0.36f, -1.13f, 0.34f, 0.15f);
    DrawWheel(3.35f, 0.36f,  1.13f, 0.34f, 0.15f);
    DrawWheel(4.95f, 0.36f, -1.00f, 0.36f, 0.16f);
    DrawWheel(4.95f, 0.36f,  1.00f, 0.36f, 0.16f);
}

// ======================== КРАСНАЯ ГОНОЧНАЯ МАШИНА =========================
void DrawWindshield(float x)
{
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    SetMaterial(0.10f, 0.32f, 0.54f, 75.0f, 0.46f, 0.52f, 0.62f, alpha ? 0.58f : 1.0f);

    // Лобовое стекло находится на передней стенке салона, а не сбоку.
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3f(x - 0.372f, 0.98f, -0.39f);
    glVertex3f(x - 0.372f, 0.98f,  0.39f);
    glVertex3f(x - 0.372f, 1.29f,  0.34f);
    glVertex3f(x - 0.372f, 1.29f, -0.34f);
    glEnd();

    // Боковые стёкла салона.
    DrawBox(x + 0.08f, 1.15f, -0.500f, 0.62f, 0.26f, 0.012f);
    DrawBox(x + 0.08f, 1.15f,  0.500f, 0.62f, 0.26f, 0.012f);

    if (alpha)
    {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void DrawEyes(float x)
{
    // Глаза располагаются поверх лобового стекла и смотрят вперёд.
    SetMaterial(0.95f, 0.96f, 0.98f, 18.0f, 0.15f, 0.15f, 0.15f);
    DrawSphere(x - 0.392f, 1.15f, -0.18f, 0.085f);
    DrawSphere(x - 0.392f, 1.15f,  0.18f, 0.085f);
    SetMaterial(0.05f, 0.07f, 0.10f, 8.0f, 0.05f, 0.05f, 0.05f);
    DrawSphere(x - 0.474f, 1.15f, -0.18f, 0.039f);
    DrawSphere(x - 0.474f, 1.15f,  0.18f, 0.039f);
}
void DrawHeadlights(float x)
{
    if (headlightsOn)
        SetEmissiveMaterial(1.0f, 0.93f, 0.52f);
    else
        SetMaterial(0.75f, 0.74f, 0.58f, 40.0f, 0.35f, 0.35f, 0.30f);

    DrawSphere(x - 1.08f, 0.82f, -0.34f, 0.10f);
    DrawSphere(x - 1.08f, 0.82f,  0.34f, 0.10f);

    // Полупрозрачные конусы света: хорошо демонстрируют alpha blending.
    if (headlightsOn && alpha)
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glColor4f(1.0f, 0.92f, 0.40f, 0.22f);
        glBegin(GL_TRIANGLES);
        glVertex3f(x - 1.13f, 0.82f, -0.34f);
        glVertex3f(x - 2.30f, 0.12f, -0.75f);
        glVertex3f(x - 2.30f, 0.12f, -0.05f);
        glVertex3f(x - 1.13f, 0.82f,  0.34f);
        glVertex3f(x - 2.30f, 0.12f,  0.05f);
        glVertex3f(x - 2.30f, 0.12f,  0.75f);
        glEnd();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        if (lightning) glEnable(GL_LIGHTING);
    }
}

float GetCarVerticalOffset()
{
    // Пока машина внутри трейлера, она остаётся выше.
    // Во время выезда плавно опускаем её на уровень дороги.
    const float startDropAt = 2.10f;
    const float finishDropAt = 4.70f;
    float t = (carTravel - startDropAt) / (finishDropAt - startDropAt);
    t = (((0.0f) > ((((1.0f) < (t)) ? (1.0f) : (t)))) ? (0.0f) : ((((1.0f) < (t)) ? (1.0f) : (t))));
    return -0.125f * t;
}

void DrawRaceCar()
{
    const float x = 1.55f - carTravel;
    const float y = GetCarVerticalOffset();
    glDisable(GL_TEXTURE_2D);

    // Корпус: красный блестящий материал.
    SetMaterial(0.80f, 0.035f, 0.025f, 95.0f, 0.90f, 0.55f, 0.46f);
    DrawBox(x,         0.70f + y, 0.0f, 2.18f, 0.36f, 1.10f);
    DrawBox(x - 0.65f, 0.91f + y, 0.0f, 0.76f, 0.20f, 1.03f); // капот
    DrawBox(x + 0.10f, 1.07f + y, 0.0f, 0.92f, 0.46f, 0.97f); // салон
    DrawBox(x + 0.85f, 0.86f + y, 0.0f, 0.33f, 0.22f, 1.03f); // задняя часть

    // Молния/декоративная жёлтая полоса на боку.
    SetMaterial(0.97f, 0.70f, 0.06f, 45.0f, 0.35f, 0.28f, 0.10f);
    DrawBox(x - 0.10f, 0.77f + y, -0.563f, 0.86f, 0.065f, 0.015f);
    DrawBox(x - 0.10f, 0.77f + y,  0.563f, 0.86f, 0.065f, 0.015f);

    glPushMatrix();
    glTranslatef(0.0f, y, 0.0f);
    DrawWindshield(x);
    DrawEyes(x);
    DrawHeadlights(x);
    glPopMatrix();

    DrawWheel(x - 0.68f, 0.39f + y, -0.61f, 0.27f, 0.13f);
    DrawWheel(x - 0.68f, 0.39f + y,  0.61f, 0.27f, 0.13f);
    DrawWheel(x + 0.66f, 0.39f + y, -0.61f, 0.27f, 0.13f);
    DrawWheel(x + 0.66f, 0.39f + y,  0.61f, 0.27f, 0.13f);
}

// ========================== УПРАВЛЕНИЕ ====================================
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
    switch (key)
    {
    case 'L': lightning = !lightning; break;
    case 'T': texturing = !texturing; break;
    case 'A': alpha = !alpha; break;
    case 'O':
        if (carTravel < 0.01f && !carDeparture)
            rampRequested = !rampRequested;
        break;
    case 'M':
        rampRequested = true;
        carDeparture = !carDeparture;
        break;
    case 'H': headlightsOn = !headlightsOn; break;
    case 'R':
        rampRequested = false;
        carDeparture = false;
        headlightsOn = false;
        rampAngle = 0.0f;
        carTravel = 0.0f;
        break;
    }
}

void UpdateScene(double delta_time)
{
    const float dt = (float)delta_time;
    const float targetRamp = rampRequested ? 105.0f : 0.0f;
    const float rampSpeed = 78.0f;

    if (rampAngle < targetRamp)
        rampAngle = (((targetRamp) < (rampAngle + rampSpeed * dt)) ? (targetRamp) : (rampAngle + rampSpeed * dt));
    else if (rampAngle > targetRamp)
        rampAngle = (((targetRamp) > (rampAngle - rampSpeed * dt)) ? (targetRamp) : (rampAngle - rampSpeed * dt));

    // Автомобиль едет только после того, как пандус почти полностью разложен.
    if (carDeparture && rampAngle > 101.0f)
    {
        carTravel = (((5.20f) < (carTravel + 1.25f * dt)) ? (5.20f) : (carTravel + 1.25f * dt));
        if (carTravel >= 5.20f)
            carDeparture = false;
    }
}

// ========================= ИНИЦИАЛИЗАЦИЯ ==================================
void initRender()
{
    trailerLogo.LoadTexture("textures/trailer_logo.png");
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    camera.caclulateCameraPos();
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);

    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
    gl.KeyDownEvent.reaction(switchModes);

    text.setSize(600, 190);
    camera.setPosition(1.0, 11.5, 3.8);
}

// ============================== РЕНДЕР =====================================
void Render(double delta_time)
{
    full_time += delta_time;
    UpdateScene(delta_time);

    if (gl.isKeyPressed('F'))
        light.SetPosition(camera.x(), camera.y(), camera.z());

    camera.SetUpCamera();
    light.SetUpLight();
    gl.DrawAxes();

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    if (lightning) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);

    // Переносим вертикаль сцены с зелёной оси Y на синюю ось Z.
    // Вращение выполняется вокруг красной оси X:
    // Y (зелёная) -> Z (синяя), а дорога оказывается в плоскости X/Y.
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(-0.70f, 0.0f, 0.0f);
    DrawGround();
    DrawTrailer();
    DrawRaceCar();
    glPopMatrix();

    // Рисуем маркер источника света.
    glLoadIdentity();
    camera.SetUpCamera();
    light.DrawLightGizmo();

    // ========================= ПОДСКАЗКА НА ЭКРАНЕ ========================
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::wstringstream ss;
    ss << L"Сцена: красная гоночная машина выезжает из трейлера\n"
       << L"M - начать/остановить выезд    O - открыть/закрыть пандус    R - сброс\n"
       << L"H - фары: " << (headlightsOn ? L"[вкл]" : L"[выкл]")
       << L"    T - текстура: " << (texturing ? L"[вкл]" : L"[выкл]")
       << L"    L - свет: " << (lightning ? L"[вкл]" : L"[выкл]")
       << L"    A - alpha: " << (alpha ? L"[вкл]" : L"[выкл]") << L"\n"
       << L"F - свет из камеры; G / G+ЛКМ - перемещение источника света\n"
       << L"Пандус: " << std::fixed << std::setprecision(1) << rampAngle
       << L" deg    Выезд машины: " << std::setprecision(2) << carTravel;

    text.setPosition(10, gl.getHeight() - 10 - 190);
    text.setText(ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
