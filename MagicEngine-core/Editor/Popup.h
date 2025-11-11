/******************************************************************************/
/*!
\file   Popup.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  ImGui popup that can display a variety of text to screen.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/ECS.h"
#include "Editor/Containers/GUIAsECS.h"

namespace editor {

	/*****************************************************************//*!
	\class Popup
	\brief
		The primary class for popup.
		Taking some code from GUIAsECS because we're not a WindowBase...
		Also making this an ecs system so it can process itself.
	*//******************************************************************/
	class Popup : public ecs::System<Popup>, public gui::PopupWindow
	{
	public:
		/*****************************************************************//*!
		\brief
			Default constructor.
		*//******************************************************************/
		Popup();

		/*****************************************************************//*!
		\brief
			Processes relevant events and draws the popup.
		*//******************************************************************/
		bool PreRun() override;

		/*****************************************************************//*!
		\brief
			Opens the popup with content specified in parameters.
		\param title
			Title of the popup.
		\param content
			String stream to display within the popup window.
		\param flags
			The flags of the popup
		*//******************************************************************/
		void Open(const std::string& title, const std::string& message, gui::PopupWindow::FLAG flags = gui::PopupWindow::FLAG::NONE);

		/*****************************************************************//*!
		\brief
			Close the popup window.
		*//******************************************************************/
		void Close();

	private:
		/*****************************************************************//*!
		\brief
			Sets this popup window's style.
		*//******************************************************************/
		void DrawContainer(int id) final;

		/*****************************************************************//*!
		\brief
			Draws the contents of this popup.
		*//******************************************************************/
		void DrawContents() final;

	private:
		std::string content;

	private:
		/*****************************************************************//*!
		\brief
			Schedules adding a system that will process this kind of component.
		*//******************************************************************/
		static bool RegisterWindowType();

		//! CRTP, execute per window type.
		inline static bool isRegistered{ RegisterWindowType() };

	};

}
