/******************************************************************************/
/*!
\file   Popup.cpp
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
#include "Editor/Popup.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"

namespace editor {

	Popup::Popup()
		: gui::PopupWindow{ "", gui::Vec2{ 500, 250 } }
	{
	}

	bool Popup::PreRun()
	{
		// Process request to open/close popup, but only the first one.
		if (const auto* openEvent{ EventsReader<Events::PopupOpenRequest>{}.ExtractEvent() })
			Open(openEvent->title, openEvent->message, openEvent->flags);
		if (const auto* closeEvent{ EventsReader<Events::PopupCloseRequest>{}.ExtractEvent() })
			Close();

		// Draw the popup.
		Draw();
		return false; // No components to process
	}

	void Popup::Open(const std::string& t, const std::string& message, gui::PopupWindow::FLAG flgs)
	{
		isOpen = true;
		title = t;
		content = message;
		flags = flgs;
	}

	void Popup::Close()
	{
		isOpen = false;
	}

	void Popup::DrawContainer(int id)
	{
		gui::SetStyleColor styleColor{ gui::FLAG_STYLE_COLOR::WINDOW_BG, gui::Vec4{ 0.0f, 0.0f, 0.0f, 0.5f } };
		gui::PopupWindow::DrawContainer(id);
	}

	void Popup::DrawContents()
	{
		gui::Separator();
		gui::TextWrapped("%s", content.c_str());
	}

}
