#include "measurement.h"
#include "core.h"
#include "util.h"
#include "globals.h"

// HLSDK
#include "keydefs.h"

CMeasurement &g_Measurement = CMeasurement::GetInstance();

void CMeasurement::UpdatePointOrigin(vec3_t &destPoint, const vec3_t &srcPoint)
{
    switch (m_iSnapMode)
    {
    case SNAPMODE_AXIS_X:
        destPoint.x = srcPoint.x;
        break;
    case SNAPMODE_AXIS_Y:
        destPoint.y = srcPoint.y;
        break;
    case SNAPMODE_AXIS_Z:
        destPoint.z = srcPoint.z;
        break;
    default:
        destPoint = srcPoint;
        break;
    }
}

void CMeasurement::TraceAlongNormal(pmtrace_t &traceData, float traceLength)
{
    vec3_t traceOrigin      = traceData.endpos;
    vec3_t planeNormal      = traceData.plane.normal;
    vec3_t *pointsList[2]   = { &m_vecPointA, &m_vecPointB };

    for (int i = 0; i < 2; ++i)
    {
        float directionSign = -1.0f + 2.0f * (i % 2);
        vec3_t traceDir = planeNormal * directionSign;
        TraceLine(traceOrigin, traceDir, traceLength, &traceData);
        *pointsList[i] = traceData.endpos;
    }
}

bool CMeasurement::WorldToScreen(int w, int h, int &x, int &y, vec3_t &origin)
{
    Vector2D screenCoords;
    if (!g_pClientEngFuncs->pTriAPI->WorldToScreen(origin, &screenCoords.x))
    {
        x = static_cast<int>((1.0f + screenCoords.x) * w * 0.5f);
        y = static_cast<int>((1.0f - screenCoords.y) * h * 0.5f);
        return true;
    }
    return false;
}

const vec3_t& CMeasurement::GetPointOriginA()
{
    return m_vecPointA;
}

const vec3_t& CMeasurement::GetPointOriginB()
{
    return m_vecPointB;
}

float CMeasurement::GetPointsDistance()
{
    return (m_vecPointB - m_vecPointA).Length();
}

void CMeasurement::HandleInput(int keyCode)
{
    int currentMode;
    vec3_t viewDir;
    vec3_t viewOrigin;
    vec3_t viewAngles;
    pmtrace_t traceData;
    const float traceLen = 64000.f;
    
    currentMode = (int)gsm_mode->value;
    if (currentMode != DISPLAYMODE_MEASUREMENT)
        return;

    if (keyCode >= K_MOUSE1 && keyCode <= K_MOUSE3)
    {
        g_pClientEngFuncs->pfnPlaySoundByName("buttons/lightswitch2.wav", 1.0f);
        g_pClientEngFuncs->GetViewAngles(viewAngles);
        g_pClientEngFuncs->pfnAngleVectors(viewAngles, viewDir, nullptr, nullptr);
        viewOrigin = g_pPlayerMove->origin + g_pPlayerMove->view_ofs;
        TraceLine(viewOrigin, viewDir, traceLen, &traceData);

        if (keyCode == K_MOUSE1)
            UpdatePointOrigin(m_vecPointA, traceData.endpos);
        else if (keyCode == K_MOUSE2)
            UpdatePointOrigin(m_vecPointB, traceData.endpos);
        else if (keyCode == K_MOUSE3)
            TraceAlongNormal(traceData, traceLen);
    }
    else if (keyCode == 'v')
    {
        ++m_iSnapMode;
        if (m_iSnapMode == SNAPMODE_MAX)
            m_iSnapMode = SNAPMODE_FREE;
        g_pClientEngFuncs->pfnPlaySoundByName("buttons/blip1.wav", 0.8f);
    }
}

void CMeasurement::DrawMeasurementLine(float lifeTime)
{
    const float lineWidth       = 2.0f;
    const float lineBrightness  = 1.0f;
    const float lineSpeed       = 5.0f;
    const float lineColorR      = 0.0f;
    const float lineColorG      = 1.0f;
    const float lineColorB      = 0.0f;

    g_pClientEngFuncs->pEfxAPI->R_BeamPoints(
        m_vecPointA, m_vecPointB, m_iLineSprite,
        lifeTime * 2.f, lineWidth, 0,
        lineBrightness, lineSpeed, 0, 0,
        lineColorR, lineColorG, lineColorB
    );
}

void CMeasurement::DrawPointHints(int screenWidth, int screenHeight)
{
    int screenX;
    int screenY;
    const int textColorR = 0;
    const int textColorG = 255;
    const int textColorB = 255;

    if (WorldToScreen(screenWidth, screenHeight, screenX, screenY, m_vecPointA))
    {
        g_pClientEngFuncs->pfnDrawString(
            screenX, screenY, "A", textColorR, textColorG, textColorB);
    }

    if (WorldToScreen(screenWidth, screenHeight, screenX, screenY, m_vecPointB))
    {
        g_pClientEngFuncs->pfnDrawString(
            screenX, screenY, "B", textColorR, textColorG, textColorB);
    }
}

void CMeasurement::DrawSupportLines(float lifeTime)
{
    vec3_t axisVector               = { 0.f, 0.f, 0.f };
    const vec3_t *pointsList[2]     = { &m_vecPointA, &m_vecPointB };
    const float lineWidth           = 1.0f;
    const float lineLenght          = 24.0f;
    const float lineSpeed           = 5.0f;
    const float lineColorR          = 1.0f;
    const float lineColorG          = 0.2f;
    const float lineColorB          = 0.2f;
    const float lineBrightness      = 1.0f;

    if (m_iSnapMode == SNAPMODE_AXIS_X)
        axisVector.x = 1.f;
    else if (m_iSnapMode == SNAPMODE_AXIS_Y)
        axisVector.y = 1.f;
    else if (m_iSnapMode == SNAPMODE_AXIS_Z)
        axisVector.z = 1.f;

    for (int i = 0; i < 2; ++i)
    {
        g_pClientEngFuncs->pEfxAPI->R_BeamPoints(
            *pointsList[i] + (axisVector * lineLenght),
            *pointsList[i] - (axisVector * lineLenght),
            m_iLineSprite,
            lifeTime * 2.0f, lineWidth, 0,
            lineBrightness, lineSpeed, 0, 0,
            lineColorR, lineColorG, lineColorB
        );
    }
}

void CMeasurement::LoadLineSprite()
{
    const char *spritePath = "sprites/laserbeam.spr";
    g_pClientEngFuncs->pfnSPR_Load(spritePath);
    m_iLineSprite = 
        g_pClientEngFuncs->pEventAPI->EV_FindModelIndex(spritePath);
}

void CMeasurement::Visualize(int screenWidth, int screenHeight)
{
    float lifeTime;
    static float lastTime;

    // disable visualisation when points not set
    if (m_vecPointA.Length() <= 0.0001f || m_vecPointB.Length() <= 0.0001f)
        return;

    if (!m_iLineSprite || g_pPlayerMove->time < lastTime)
        LoadLineSprite();

    lastTime = g_pPlayerMove->time;
    lifeTime = min(0.05f, g_pPlayerMove->frametime);

    DrawMeasurementLine(lifeTime);
    DrawPointHints(screenWidth, screenHeight);

    if (m_iSnapMode != SNAPMODE_FREE)
        DrawSupportLines(lifeTime);
}

const char *CMeasurement::GetSnapModeName()
{
    switch (m_iSnapMode)
    {
    case SNAPMODE_FREE:
        return "Free";
        break;
    case SNAPMODE_AXIS_X:
        return "Axis X";
        break;
    case SNAPMODE_AXIS_Y:
        return "Axis Y";
        break;
    case SNAPMODE_AXIS_Z:
        return "Axis Z";
        break;
    }
    return "";
}

float CMeasurement::GetLineElevationAngle()
{
    vec3_t highPoint;
    vec3_t lowPoint;
    vec3_t lineDirection;

    if (m_vecPointA.z > m_vecPointB.z)
    {
        highPoint = m_vecPointA;
        lowPoint  = m_vecPointB;
    }
    else
    {
        highPoint = m_vecPointB;
        lowPoint  = m_vecPointA;
    }

    lineDirection = highPoint - lowPoint;
    lineDirection = lineDirection.Normalize();
    return asinf(lineDirection.z) * 180.0f / 3.14f;
}

CMeasurement &CMeasurement::GetInstance()
{
    static CMeasurement instance;
    return instance;
}
