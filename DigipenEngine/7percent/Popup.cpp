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
#include "Popup.h"

Popup::Popup()
	: gui::PopupWindow{ "", gui::Vec2{ 500, 250 } }
{
}

void Popup::Open(const std::string& t, const std::string& message, gui::PopupWindow::FLAG flgs)
{
	isOpen = true;
	title = t;
	content = message;
	flags = flgs;
}

void Popup::OpenWithContent(const std::string& t, const std::ostringstream& c)
{
	isOpen = true;
	title = t;
	content = c.str();
	flags = gui::PopupWindow::FLAG::NONE;
}

void Popup::Close()
{
	isOpen = false;
}

void Popup::DrawContainer()
{
	gui::SetStyleColor styleColor{ gui::FLAG_STYLE_COLOR::WINDOW_BG, gui::Vec4{ 0.0f, 0.0f, 0.0f, 0.5f } };
	gui::PopupWindow::DrawContainer();
}

void Popup::DrawContents()
{
	gui::Separator();
	gui::TextWrapped("%s", content.c_str());
}
