/******************************************************************************/
/*!
\file   CameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of CameraController.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "CameraController.h"
#include "GameSettings.h"

CameraController::CameraController()
{
}

const CameraData& CameraController::GetCameraData() const
{
    return m_cameraData;
}
void CameraController::SetCameraData(const CameraData& data)
{
    m_cameraData = data;
}

const Vector2& CameraController::GetPosition() const
{
    return m_cameraData.position;
}
void CameraController::SetPosition(const Vector2& position)
{
    m_cameraData.position = position;
}
void CameraController::AddPosition(const Vector2& position)
{
    m_cameraData.position += position;
}

float CameraController::GetZoom() const
{
    return m_cameraData.zoom;
}
void CameraController::SetZoom(float zoom)
{
    zoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    m_cameraData.zoom = m_cameraData.targetZoom = zoom;
}
void CameraController::AddZoom(float zoom)
{
    float newZoom = std::clamp(m_cameraData.zoom + zoom, MIN_ZOOM, MAX_ZOOM);
    m_cameraData.zoom = newZoom;
    m_cameraData.targetZoom = std::clamp(m_cameraData.targetZoom + zoom, MIN_ZOOM, MAX_ZOOM);
}

float CameraController::GetRotation() const
{
    return m_cameraData.rotation;
}

void CameraController::SetRotation(float rotation) 
{
    m_cameraData.rotation = rotation;
}

void CameraController::AddRotation(float rotation)
{
    m_cameraData.rotation += rotation;
}

void CameraController::SetTargetZoom(float zoom)
{
    m_cameraData.targetZoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
}
void CameraController::AddTargetZoom(float zoom)
{
    m_cameraData.targetZoom = std::clamp(m_cameraData.targetZoom + zoom, MIN_ZOOM, MAX_ZOOM);
}

void CameraController::MultTargetZoom(float mult)
{
    m_cameraData.targetZoom = std::clamp(m_cameraData.targetZoom * mult, MIN_ZOOM, MAX_ZOOM);
}

void CameraController::LerpZoom(float dt)
{
    // Don't lerp camera if zoom is close enough
    if (fabs(m_cameraData.targetZoom - m_cameraData.zoom) <= std::numeric_limits<float>::epsilon())
        return;

    // Lerp the camera zoom towards the target zoom
    float prevZoom = m_cameraData.zoom;
    float newZoom = util::Lerp(prevZoom, m_cameraData.targetZoom, ST<GameSettings>::Get()->m_editorZoomLerpFactor, dt);
    m_cameraData.zoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);
}

float CameraController::GetZoomMultiplierFromInput(float inputAmount, float baseZoomScale)
{
    float positiveScale = 1.0f + baseZoomScale;
    float scale = (inputAmount < 0.0f ? 1.0f / positiveScale : positiveScale);
    return std::powf(scale, std::fabsf(inputAmount));
}
