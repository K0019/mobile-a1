/******************************************************************************/
/*!
\file   Camera.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	Contains the API for the camera

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
	public class Camera
	{

		/*****************************************************************//*!
		\brief
			Sets the zoom of the main camera.
		\param[in] newZoom
			Amount to zoom in.
		\return
			None.
		*//******************************************************************/
		public static void SetZoom(float newZoom)
		{
			InternalCalls.SetCameraZoom(newZoom);
		}

		/*****************************************************************//*!
		\brief
			Gets the zoom of the main camera
		\return
			Float value of the zoom.
		*//******************************************************************/
		public static float GetZoom()
		{
			return InternalCalls.GetCameraZoom();
		}
	}
}
