/******************************************************************************/
/*!
\file   GUICollection.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   12/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements an intermediate layer connecting engine
  code with ImGui code, hiding ImGui stuff that shouldn't be exposed and alleviating
  the need for engine code to #ifdef out GUI code depending on the project configuration.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "GUICollection.h"

namespace gui {

	namespace internal {

#ifdef IMGUI_ENABLED
		bool BeginChild(const char* str_id, const ImVec2& size, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
		{
			return ImGui::BeginChild(str_id, size, child_flags, window_flags);
		}
		bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
		{
			return ImGui::CollapsingHeader(label, flags);
		}
		void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
		{
			ImGui::PushStyleVar(idx, val);
		}
		void PopStyleVar()
		{
			ImGui::PopStyleVar();
		}
		void PushStyleColor(ImGuiCol idx, const ImVec4& val)
		{
			ImGui::PushStyleColor(idx, val);
		}
		void PopStyleColor()
		{
			ImGui::PopStyleColor();
		}
#endif

		ContainerBase::ContainerBase(const std::string& title, const Vec2& initialDimensions, FLAG_COND windowSizeCondFlags)
			: isOpen{ false }
			, title{ title }
			, initialDimensions{ initialDimensions }
			, windowSizeCondFlags{ windowSizeCondFlags }
		{
		}

		void ContainerBase::Draw()
		{
#ifdef IMGUI_ENABLED
			if (!isOpen)
				return;

			ImGui::SetNextWindowSize(initialDimensions, +windowSizeCondFlags);
			DrawContainer();
#endif
		}

	}

	Window::Window(const std::string& title, const Vec2& initDimensions, FLAG_WINDOW windowFlags)
		: internal::ContainerBase{ title, initDimensions, FLAG_COND::FIRST_USE_EVER }
		, windowFlags{ windowFlags }
	{
	}

	void Window::DrawContainer()
	{
#ifdef IMGUI_ENABLED
		if (ImGui::Begin(title.c_str(), &isOpen, +windowFlags))
			DrawContents();
		ImGui::End();
#endif
	}

	bool Window::GetIsOpen() const
	{
		return isOpen;
	}

	void Window::SetIsOpen(bool newIsOpen)
	{
		if (isOpen != newIsOpen)
		{
			isOpen = newIsOpen;
			OnOpenStateChanged();
		}
	}

	void Window::OnOpenStateChanged()
	{
	}

	Child::Child([[maybe_unused]] const char* str_id, [[maybe_unused]] const Vec2& size, [[maybe_unused]] FLAG_CHILD child_flags, [[maybe_unused]] FLAG_WINDOW window_flags)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Child{ str_id, size, +child_flags, +window_flags }
#endif
	{
	}

	Popup::Popup([[maybe_unused]] const char* str_id, [[maybe_unused]] FLAG_WINDOW flags)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Popup{ str_id, +flags }
#endif
	{
	}

	void Popup::Open([[maybe_unused]] const char* identifier)
	{
#ifdef IMGUI_ENABLED
		ImGui::OpenPopup(identifier);
#endif
	}

	void Popup::Close()
	{
#ifdef IMGUI_ENABLED
		ImGui::CloseCurrentPopup();
#endif
	}

	bool Popup::WasOpenedThisFrame()
	{
#ifdef IMGUI_ENABLED
		return ImGui::IsWindowAppearing();
#else
		return false;
#endif
	}

	PopupWindow::PopupWindow(const std::string& title, const Vec2& initDimensions)
		: internal::ContainerBase{ title, initDimensions, FLAG_COND::APPEARING }
		, flags{}
	{
	}

	void PopupWindow::DrawContainer()
	{
#ifdef IMGUI_ENABLED
		ImGui::OpenPopup(title.c_str());

		bool* isOpenPtr{ +(flags & FLAG::NO_CLOSE_BUTTON) ? nullptr : &isOpen };
		if (ImGui::BeginPopupModal(title.c_str(), isOpenPtr))
		{
			DrawContents();
			ImGui::EndPopup();
		}
#endif
	}

	Tooltip::Tooltip()
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Tooltip{ true }
#endif
	{
	}

	Combo::Combo([[maybe_unused]] const char* label, [[maybe_unused]] const char* previewValue, [[maybe_unused]] FLAG_COMBO flags)
#ifdef IMGUI_ENABLED
		: shouldCallEndCombo{ ImGui::BeginCombo(label, previewValue, +flags) }
#else
		: shouldCallEndCombo{ false }
#endif
	{
	}

	Combo::Combo(const char* label, const char* const* data, size_t numElems, int* selectedIndex)
		: shouldCallEndCombo{ false }
	{
		CallCombo(label, data, numElems, selectedIndex);
	}

	Combo::~Combo()
	{
#ifdef IMGUI_ENABLED
		if (shouldCallEndCombo)
			ImGui::EndCombo();
#endif
	}

	bool Combo::Selectable([[maybe_unused]] const char* label, [[maybe_unused]] bool isSelected)
	{
#ifdef IMGUI_ENABLED
		return ImGui::Selectable(label, isSelected);
#else
		return false;
#endif
	}

	Combo::operator bool() const
	{
		return selectionHappened || shouldCallEndCombo;
	}

	void Combo::CallCombo([[maybe_unused]] const char* label, [[maybe_unused]] const char* const* data, [[maybe_unused]] size_t numElems, [[maybe_unused]] int* selectedIndex)
	{
#ifdef IMGUI_ENABLED
		selectionHappened = ImGui::Combo(label, selectedIndex, data, static_cast<int>(numElems));
#endif
	}

	SetID::SetID([[maybe_unused]] int id)
	{
#ifdef IMGUI_ENABLED
		ImGui::PushID(id);
#endif
	}
	SetID::SetID([[maybe_unused]] const char* label)
	{
#ifdef IMGUI_ENABLED
		ImGui::PushID(label);
#endif
	}

	SetID::~SetID()
	{
#ifdef IMGUI_ENABLED
		ImGui::PopID();
#endif
	}

	Button::Button([[maybe_unused]] const char* label, [[maybe_unused]] const Vec2& size)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Button{ label, size }
#endif
	{
	}

	Menu::Menu([[maybe_unused]] const char* label)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Menu{ label, true }
#endif
	{
	}

	CollapsingHeader::CollapsingHeader([[maybe_unused]] const char* label, [[maybe_unused]] FLAG_TREE_NODE flags)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_CollapsingHeader{ label, +flags }
#endif
	{
	}
	CollapsingHeader::CollapsingHeader(const std::string& label, FLAG_TREE_NODE flags)
		: CollapsingHeader{ label.c_str(), flags }
	{
	}

	Table::Table([[maybe_unused]] const char* label, [[maybe_unused]] int numCols, bool hasHeader, [[maybe_unused]] FLAG_TABLE flags, [[maybe_unused]] const Vec2& outerSize, [[maybe_unused]] float innerWidth)
		: tableFlags{ flags }
#ifdef IMGUI_ENABLED
		, internal::BeginEndBound_Table{ label, numCols, +flags, outerSize, innerWidth }
#endif
	{
		// ImGui requires TableNextColumn() to be called before data of first column.
		if (!hasHeader)
			NextColumn();
	}

	void Table::AddColumnHeader([[maybe_unused]] const char* label, [[maybe_unused]] FLAG_TABLE_COLUMN flags, [[maybe_unused]] float width)
	{
#ifdef IMGUI_ENABLED
		ImGui::TableSetupColumn(label, +flags, width);
#endif
	}

	void Table::SubmitColumnHeaders()
	{
#ifdef IMGUI_ENABLED
		if (!+(tableFlags & FLAG_TABLE::HIDE_HEADER))
			ImGui::TableHeadersRow();
		NextColumn();
#endif
	}

	void Table::NextColumn()
	{
#ifdef IMGUI_ENABLED
		ImGui::TableNextColumn();
#endif
	}

	float GetFrameHeightWithSpacing()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetFrameHeightWithSpacing();
#else
		return 1.0f;
#endif
	}

	bool IsCurrWindowFocused()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetCurrentContext()->WindowsFocusOrder.Size == ImGui::GetCurrentContext()->CurrentWindow->FocusOrder + 1;
#else
		return true;
#endif
	}

	void SetItemDefaultFocus()
	{
#ifdef IMGUI_ENABLED
		ImGui::SetItemDefaultFocus();
#endif
	}

	float GetFontSize()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetFontSize();
#else
		return 1.0f;
#endif
	}

	void AlignTextToFramePadding()
	{
#ifdef IMGUI_ENABLED
		ImGui::AlignTextToFramePadding();
#endif
	}

	void TextUnformatted([[maybe_unused]] const char* text)
	{
#ifdef IMGUI_ENABLED
		ImGui::TextUnformatted(text);
#endif
	}
	void TextUnformatted(const std::string& text)
	{
		TextUnformatted(text.c_str());
	}

	void TextCenteredUnformatted([[maybe_unused]] const char* text)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f);
		TextUnformatted(text);
#endif
	}

	void TextCenteredUnformatted(const std::string& text)
	{
		TextCenteredUnformatted(text.c_str());
	}

	void TextBoxReadOnly([[maybe_unused]] const char* label, [[maybe_unused]] const char* text, [[maybe_unused]] size_t size)
	{
#ifdef IMGUI_ENABLED
		ImGui::InputText(label, const_cast<char*>(text), size, +FLAG_INPUT_TEXT::READ_ONLY);
#endif
	}
	void TextBoxReadOnly(const char* label, const std::string& text)
	{
		TextBoxReadOnly(label, text.data(), text.size() + 1);
	}

	SetTextWrapPos::SetTextWrapPos([[maybe_unused]] float wrap_local_pos_x)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_TextWrapPos{ wrap_local_pos_x }
#endif
	{
	}

	TextBoxWithFilter::TextBoxWithFilter(bool matchStarting)
		: matchStarting{ matchStarting }
	{
	}

	void TextBoxWithFilter::Draw([[maybe_unused]] const char* label, [[maybe_unused]] float width)
	{
#ifdef IMGUI_ENABLED
		ImGuiTextFilter::Draw(label, width);
#endif
	}

	bool TextBoxWithFilter::PassFilter([[maybe_unused]] const char* text) const
	{
#ifdef IMGUI_ENABLED
		// Whether to use ImGui's in-built filter
		if (!matchStarting)
			return ImGuiTextFilter::PassFilter(text);

		// Candidates cannot pass the filter if it is shorter than the filter
		size_t filterLength{ std::strlen(InputBuf) };
		if (filterLength && std::strlen(text) < filterLength)
			return false;

		// This checks up to the number of characters within the filter
		return std::equal(InputBuf, InputBuf + filterLength, text, text + filterLength,
			[](char a, char b) -> bool { return std::tolower(a) == std::tolower(b); });
#else
		return false;
#endif
	}

	void TextBoxWithFilter::Clear()
	{
#ifdef IMGUI_ENABLED
		InputBuf[0] = '\0';
		CountGrep = 0;
#endif
	}

	bool TextBoxWithFilter::IsEmpty() const
	{
#ifdef IMGUI_ENABLED
			return InputBuf[0] == '\0';
#else
		return false;
#endif
	}

	bool Selectable([[maybe_unused]] const char* label, [[maybe_unused]] bool isSelected)
	{
#ifdef IMGUI_ENABLED
		return ImGui::Selectable(label, isSelected);
#else
		return false;
#endif
	}

	bool Checkbox([[maybe_unused]] const char* label, [[maybe_unused]] bool* v)
	{
#ifdef IMGUI_ENABLED
		return ImGui::Checkbox(label, v);
#else
		return false;
#endif
	}

	bool VarDrag([[maybe_unused]] const char* label, [[maybe_unused]] int* v, [[maybe_unused]] float speed, [[maybe_unused]] int min, [[maybe_unused]] int max)
	{
#ifdef IMGUI_ENABLED
		return ImGui::DragInt(label, v, speed, min, max);
#else
		return false;
#endif
	}
	bool VarDrag([[maybe_unused]] const char* label, [[maybe_unused]] float* v, [[maybe_unused]] float speed, [[maybe_unused]] float min, [[maybe_unused]] float max, [[maybe_unused]] const char* format)
	{
#ifdef IMGUI_ENABLED
		return ImGui::DragFloat(label, v, speed, min, max, format);
#else
		return false;
#endif
	}
	bool VarDrag([[maybe_unused]] const char* label, [[maybe_unused]] Vector2* v, [[maybe_unused]] float speed, [[maybe_unused]] Vector2 min, [[maybe_unused]] Vector2 max, [[maybe_unused]] const char* format)
	{
#ifdef IMGUI_ENABLED
		bool modified = false;
		if (Table table{ label, 3, false, FLAG_TABLE::HIDE_HEADER })
		{
			const auto DrawFloatComponent{ [&modified, speed, format](const char* text, const char* label, float* value, float min, float max, const Vec4& textColor) -> void {
				{
					SetStyleColor styleColText{ FLAG_STYLE_COLOR::TEXT, textColor };
					TextUnformatted(text);
				}
				SameLine();
				SetNextItemWidth(GetAvailableContentRegion().x);
				modified |= VarDrag(label, value, speed, min, max, format);
			} };

			DrawFloatComponent("X", "##X", &v->x, min.x, max.x, Vec4{ 1.0f, 0.4f, 0.4f, 1.0f });
			table.NextColumn();
			DrawFloatComponent("Y", "##Y", &v->y, min.y, max.y, Vec4{ 0.4f, 1.0f, 0.4f, 1.0f });
			table.NextColumn();
			TextUnformatted(label);
		}
		return modified;
#else
		return false;
#endif
	}

	bool VarInput([[maybe_unused]] const char* label, [[maybe_unused]] int* v, [[maybe_unused]] int step)
	{
#ifdef IMGUI_ENABLED
		return ImGui::InputInt(label, v, step, step * 10);
#else
		return false;
#endif
	}

	bool& GetVar([[maybe_unused]] const char* label, [[maybe_unused]] bool defaultVal)
	{
#ifdef IMGUI_ENABLED
		return *ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID(label), defaultVal);
#else
		static bool dummy{};
		return dummy;
#endif
	}

	// NOTE: Should consider finding a way to decrease code duplication for the different types...
	template<>
	bool Slider<int>([[maybe_unused]] const char* label, [[maybe_unused]] int* v, [[maybe_unused]] const int& min, [[maybe_unused]] const int& max)
	{
#ifdef IMGUI_ENABLED
		return ImGui::SliderInt(label, v, min, max);
#else
		return false;
#endif
	}
	template<>
	bool Slider<float>([[maybe_unused]] const char* label, [[maybe_unused]] float* v, [[maybe_unused]] const float& min, [[maybe_unused]] const float& max)
	{
#ifdef IMGUI_ENABLED
		return ImGui::SliderFloat(label, v, min, max);
#else
		return false;
#endif
	}

	template <>
	void PayloadSource<std::string>([[maybe_unused]] const char* identifier, [[maybe_unused]] const std::string& data, [[maybe_unused]] const char* dragLabel)
	{
#ifdef IMGUI_ENABLED
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(identifier, data.c_str(), data.size() + 1);
			ImGui::Text("Dragging %s", dragLabel);
			ImGui::EndDragDropSource();
		}
#endif
	}

	bool MenuItem([[maybe_unused]] const char* label, [[maybe_unused]] bool* p_selected)
	{
#ifdef IMGUI_ENABLED
		return ImGui::MenuItem(label, nullptr, p_selected);
#else
		return false;
#endif
	}

	bool IsItemHovered()
	{
#ifdef IMGUI_ENABLED
		return ImGui::IsItemHovered();
#else
		return false;
#endif
	}

	Indent::Indent(float amt)
		: indentAmt{ amt }
	{
#ifdef IMGUI_ENABLED
		ImGui::Indent(indentAmt);
#endif
	}

	Indent::~Indent()
	{
#ifdef IMGUI_ENABLED
		ImGui::Unindent(indentAmt);
#endif
	}

	void Separator()
	{
#ifdef IMGUI_ENABLED
		ImGui::Separator();
#endif
	}

	void Spacing()
	{
#ifdef IMGUI_ENABLED
		ImGui::Spacing();
#endif
	}

	void SameLine([[maybe_unused]] float offset_from_start_x, [[maybe_unused]] float spacing)
	{
#ifdef IMGUI_ENABLED
		ImGui::SameLine(offset_from_start_x, spacing);
#endif
	}

	Vec2 GetAvailableContentRegion()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetContentRegionAvail();
#else
		return Vec2{ 1.0f, 1.0f };
#endif
	}

	Vec2 GetPrevItemRectSize()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetItemRectSize();
#else
		return Vec2{ 1.0f, 1.0f };
#endif
	}
	Vec2 GetPrevItemRectMin()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetItemRectMin();
#else
		return Vec2{ 1.0f, 1.0f };
#endif
	}
	Vec2 GetPrevItemRectMax()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetItemRectMax();
#else
		return Vec2{ 1.0f, 1.0f };
#endif
	}

	void SetNextItemWidth([[maybe_unused]] float width)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetNextItemWidth(width);
#endif
	}

	bool IsKeyPressed([[maybe_unused]] KEY key, [[maybe_unused]] bool repeating)
	{
#ifdef IMGUI_ENABLED
		return ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), repeating);
#else
		return false;
#endif
	}

	void SetKeyboardFocusHere([[maybe_unused]] int offset)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetKeyboardFocusHere(offset);
#endif
	}

	void SetScrollHereY([[maybe_unused]] float center_y_ratio)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetScrollHereY(center_y_ratio);
#endif
	}

	float GetWindowWidth()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetWindowWidth();
#else
		return 0.0f;
#endif
	}

	float GetWindowHeight()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetWindowHeight();
#else
		return 0.0f;
#endif
	}

	Vec2 GetWindowContentRegionMax()
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetWindowContentRegionMax();
#else
		return Vec2{ 1.0f, 1.0f };
#endif
	}

	void SetNextWindowPos([[maybe_unused]] const Vec2& pos)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetNextWindowPos(pos);
#endif
	}
	void SetNextWindowSize([[maybe_unused]] const Vec2& size)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetNextWindowSize(size);
#endif
	}

	void SetNextWindowSizeConstraints([[maybe_unused]] const Vec2& min, [[maybe_unused]] const Vec2& max)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetNextWindowSizeConstraints(min, max);
#endif
	}

	SetStyleColor::SetStyleColor([[maybe_unused]] FLAG_STYLE_COLOR idx, [[maybe_unused]] const Vec4& val, [[maybe_unused]] bool apply)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_StyleColor{ apply, +idx, val }
#endif
	{
	}

	UnsetStyleColor::UnsetStyleColor([[maybe_unused]] unsigned int numPops)
	{
#ifdef IMGUI_ENABLED
		auto context{ ImGui::GetCurrentContext() };
		for (; numPops; --numPops)
		{
			popped.push_back(context->ColorStack.back());
			ImGui::PopStyleColor();
		}
#endif
	}

	UnsetStyleColor::~UnsetStyleColor()
	{
#ifdef IMGUI_ENABLED
		while (!popped.empty())
		{
			const auto& styleCol{ popped.back() };
			ImGui::PushStyleColor(styleCol.Col, styleCol.BackupValue);
			popped.pop_back();
		}
#endif
	}

	SetStyleVar::SetStyleVar([[maybe_unused]] FLAG_STYLE_VAR idx, [[maybe_unused]] const Vec2& val)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_StyleVar{ +idx, val }
#endif
	{
	}

	UnsetStyleVar::UnsetStyleVar([[maybe_unused]] unsigned int numPops)
	{
#ifdef IMGUI_ENABLED
		auto context{ ImGui::GetCurrentContext() };
		for (; numPops; --numPops)
		{
			popped.push_back(context->StyleVarStack.back());
			ImGui::PopStyleVar();
		}
#endif
	}

	UnsetStyleVar::~UnsetStyleVar()
	{
#ifdef IMGUI_ENABLED
		while (!popped.empty())
		{
			const auto& styleVar{ popped.back() };
			ImGui::PushStyleVar(styleVar.VarIdx, Vec2{ styleVar.BackupFloat[0], styleVar.BackupFloat[1] });
			popped.pop_back();
		}
#endif
	}

	Vec4 GetStyleColor([[maybe_unused]] FLAG_STYLE_COLOR style)
	{
#ifdef IMGUI_ENABLED
		return ImGui::GetStyleColorVec4(+style);
#else
		return Vec4{};
#endif
	}

	void DrawTriangle([[maybe_unused]] Vec2 p0, [[maybe_unused]] Vec2 p1, [[maybe_unused]] Vec2 p2, [[maybe_unused]] const Vec4& color)
	{
#ifdef IMGUI_ENABLED
		ImGui::GetWindowDrawList()->AddTriangleFilled(p0, p1, p2, ImGui::ColorConvertFloat4ToU32(color));
#endif
	}

	ItemContextMenu::ItemContextMenu([[maybe_unused]] const char* label, [[maybe_unused]] FLAG_POPUP flags)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_ItemContextMenu{ label, +flags }
#endif
	{
	}

	Disabled::Disabled([[maybe_unused]] bool isDisabled)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Disabled{ isDisabled }
#endif
	{
	}

}
