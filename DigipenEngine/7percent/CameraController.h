#pragma once
/******************************************************************************/
/*!
\file   CameraController.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
The camera controller class that handles camera data and zooming. Also used by the camera system to update the camera.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

struct CameraData {
    Vector2 position{ 0.0f, 0.0f };
    float rotation{ 0.0f };
    float zoom{ 1.0f };
    float targetZoom{ zoom }; // The zoom amount to lerp to
};

class CameraController
{
    friend class ST<CameraController>;
    public:
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    const CameraData& GetCameraData() const;
    void SetCameraData(const CameraData& data);

    const Vector2& GetPosition() const;
    void SetPosition(const Vector2& position);
    void AddPosition(const Vector2& position);

    float GetZoom() const;
    void SetZoom(float zoom);
    void AddZoom(float zoom);

    float GetRotation() const;
    void SetRotation(float rotation);
    void AddRotation(float rotation);

    void SetTargetZoom(float zoom);
    void AddTargetZoom(float zoom);
    void MultTargetZoom(float mult);
    void LerpZoom(float dt);

    // Returns a scale amount to scale the target zoom by based on a scale factor and input value
    static float GetZoomMultiplierFromInput(float inputAmount, float baseZoomScale);

    static constexpr float MIN_ZOOM = 0.15f;
    static constexpr float MAX_ZOOM = 15.0f;

private:
    CameraController();
    CameraData m_cameraData;
};

