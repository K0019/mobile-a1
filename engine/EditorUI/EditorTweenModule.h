/******************************************************************************/
/*!
\file   EditorTweenModule.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   02/11/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Handles the interpolation of the Editor camera.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\class EditorTweenModule
\brief
	A module which, when instantiated, listens for a message and
	tweens the CameraController.
*//******************************************************************/
class EditorTweenModule
{
public:
	/*****************************************************************//*!
	\brief
		Constructor which subscribes to "OnEntitySelected" message.
	*//******************************************************************/
	EditorTweenModule();

	/*****************************************************************//*!
	\brief
		Interpolates the camera to the position of the entity and also
		zooms in to the entity.
	\param entity
		Entity to target.
	*//******************************************************************/
	static void TweenCameraToEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Destructor which unsubscribes to "OnEntitySelected" message.
	*//******************************************************************/
	~EditorTweenModule();

	// Zoom control. 1.0f means the entity fills the window.
	static constexpr float zoomAmount = 0.6f;

	// Duration of the interpolation.
	static constexpr float tweenDuration = 0.5f;
private:

};