#pragma once
#include "Editor/Containers/GUICollection.h"

namespace gui {
	namespace internal {

		template<auto StartFunc, void (*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		template<typename ...Args>
		BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::BeginEndBound(Args... args)
			: isOpen{ CallStartFuncBasedOnParams(args...) }
		{
		}

		template<auto StartFunc, void(*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		template<typename ...Args>
		bool BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::CallStartFunc(Args... args)
		{
			if constexpr (std::is_same_v<util::ReturnType_t<StartFunc>, bool>)
				return StartFunc(args...);
			else
			{
				StartFunc(args...);
				return true;
			}
		}

		template<auto StartFunc, void(*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		template<typename ...Args>
		bool BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::CallStartFuncBasedOnParams(Args... args)
		{
			if constexpr (StartIsOptional)
				return [](bool first, auto&... args) -> bool {
				if (!first)
					return false;
				return CallStartFunc(args...);
				}(args...);
			else
				return CallStartFunc(args...);
		}

		template<auto StartFunc, void (*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::~BeginEndBound()
		{
			if constexpr (static_cast<bool>(EndFunc))
				if (isOpen || AlwaysCallEnd)
					EndFunc();
		}

		template<auto StartFunc, void (*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::operator bool() const
		{
			return isOpen;
		}

		template<auto StartFunc, void(*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		bool BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::GetIsOpen()
		{
			return isOpen;
		}

	}

	template<typename ...Args>
	Tooltip::Tooltip(const char* fmt, const Args&... args)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_Tooltip{ false }
#endif
	{
#ifdef IMGUI_ENABLED
		ImGui::SetTooltip(fmt, args...);
#endif
	}

	template<size_t BufferSize>
	TextBoxWithBuffer<BufferSize>::TextBoxWithBuffer(const char* label, const std::string& text)
		: label{ label }
		, buffer{}
	{
		if (!text.empty())
			SetBuffer(text);
	}

	template<size_t BufferSize>
	bool TextBoxWithBuffer<BufferSize>::Draw([[maybe_unused]] FLAG_INPUT_TEXT flags, [[maybe_unused]] types::InputTextCallback callback)
	{
#ifdef IMGUI_ENABLED
		return ImGui::InputText(label, buffer, BufferSize, +flags, callback);
#else
		return false;
#endif
	}

	template<size_t BufferSize>
	const char* TextBoxWithBuffer<BufferSize>::GetBufferPtr() const
	{
		return buffer;
	}

	template<size_t BufferSize>
	std::string TextBoxWithBuffer<BufferSize>::GetBuffer() const
	{
		return buffer;
	}

	template<size_t BufferSize>
	void TextBoxWithBuffer<BufferSize>::SetBuffer(const std::string& text)
	{
		text.copy(buffer, BufferSize - 1);
		buffer[std::min(text.size(), BufferSize - 1)] = '\0';
	}

	template<size_t BufferSize>
	void TextBoxWithBuffer<BufferSize>::ClearBuffer()
	{
		buffer[0] = '\0';
	}

	template<typename ...Args>
	void TextFormatted([[maybe_unused]] internal::TextType fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		if constexpr (sizeof...(args) == 0)
			ImGui::TextUnformatted(fmt);
		else
			ImGui::Text(fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextColored([[maybe_unused]] const Vec4& color, [[maybe_unused]] internal::TextType fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		if constexpr (sizeof...(args) == 0)
			ImGui::TextColored(color, "%s", fmt);
		else
			ImGui::TextColored(color, fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextWrapped([[maybe_unused]] internal::TextType fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		if constexpr (sizeof...(args) == 0)
			ImGui::TextWrapped("%s", fmt);
		else
			ImGui::TextWrapped(fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextDisabled([[maybe_unused]] internal::TextType format, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		if constexpr (sizeof...(args) == 0)
			ImGui::TextDisabled("%s", format);
		else
			ImGui::TextDisabled(format, args...);
#endif
	}

	template<typename NormalFuncType, typename RenameCallbackFuncType>
	void Renamable::Wrap([[maybe_unused]] const char* currText, [[maybe_unused]] NormalFuncType normalRoute, [[maybe_unused]] RenameCallbackFuncType onRenameRoute)
	{
#ifdef IMGUI_ENABLED
		int currId{ GetCurrID() };
		if (idOfItemBeingRenamed != currId)
		{
			// Draw the user's normal draw routine
			normalRoute();

			// Add on a right click context menu
			if (ItemContextMenu contextMenu{ "GenericRenameContextMenu" })
				if (MenuItem("Rename"))
				{
					idOfItemBeingRenamed = currId;
				#ifdef __ANDROID__
					strncpy(buffer, currText, 256);
				#else
					strncpy_s(buffer, currText, 256);
				#endif
				}
		}
		else
		{
			// Draw an input text box for renaming
			SetNextItemWidth(-1.0f);
			SetID id{ "GenericRename" };
			if (ImGui::InputText("", buffer, 256, +FLAG_INPUT_TEXT::ENTER_RETURNS_TRUE))
			{
				onRenameRoute(buffer);
				idOfItemBeingRenamed = -1;
			}
			// Check for escape key pressed
			if (IsItemFocused() && IsKeyPressed(KEY::ESC))
				idOfItemBeingRenamed = -1;
		}
#endif
	}


	template<typename T, typename ElemDrawFuncType>
	bool VarContainer([[maybe_unused]] const char* label, [[maybe_unused]] std::vector<T>* v, [[maybe_unused]] ElemDrawFuncType elemDrawFunc)
	{
#ifdef IMGUI_ENABLED
		bool modified{ false };

		TextUnformatted(label);
		SameLine();

		int contSize{ static_cast<int>(v->size()) };
		if (VarInput("Size", &contSize))
		{
			v->resize(static_cast<size_t>(std::max(contSize, 0)));
			modified = true;
		}

		for (size_t i{}, end{ v->size() }; i < end; ++i)
		{
			SetID id{ static_cast<int>(i) };
			TextFormatted("%d", i);
			SameLine();
			if constexpr (std::is_same_v<decltype(elemDrawFunc(v->at(i))), void>)
				elemDrawFunc(v->at(i));
			else
				if (elemDrawFunc(v->at(i)))
					modified = true;
		}

		return modified;
#else
		return false;
#endif
	}

	template<typename DataType>
	PayloadSource<DataType>::PayloadSource([[maybe_unused]] const char* identifier, [[maybe_unused]] const DataType& data, [[maybe_unused]] const char* dragLabel)
#ifdef IMGUI_ENABLED
		: internal::BeginEndBound_PayloadSource{ 0 } // No flags
#endif
	{
#ifdef IMGUI_ENABLED
		if (GetIsOpen())
		{
			SetPayloadTarget(identifier, data);

			if (dragLabel)
				ImGui::Text("Dragging %s", dragLabel);
		}
#endif
	}

	template<typename DataType>
	void PayloadSource<DataType>::SetPayloadTarget([[maybe_unused]] const char* identifier, [[maybe_unused]] const DataType& data)
	{
#ifdef IMGUI_ENABLED
		ImGui::SetDragDropPayload(identifier, &data, sizeof(DataType));
#endif
	}

	namespace internal {
		// These classes exist to work around the limitation of no partial specializations allowed for functions by utilizing
		// the allowance of partial specializations of classes.
		template <typename DataType, typename FunctionType>
		struct PayloadTargetClass
		{
			static void Invoke([[maybe_unused]] const char* identifier, [[maybe_unused]] FunctionType onReceive)
			{
#ifdef IMGUI_ENABLED
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(identifier) })
						onReceive(*reinterpret_cast<const DataType*>(payload->Data));

					ImGui::EndDragDropTarget();
				}
#endif
			}
		};
		template <typename FunctionType>
		struct PayloadTargetClass<std::string, FunctionType>
		{
			static void Invoke([[maybe_unused]] const char* identifier, [[maybe_unused]] FunctionType onReceive)
			{
#ifdef IMGUI_ENABLED
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(identifier) })
						onReceive(reinterpret_cast<const char*>(payload->Data));

					ImGui::EndDragDropTarget();
				}
#endif
			}
		};
	}
	template<typename DataType, typename FunctionType>
		requires std::invocable<FunctionType, const DataType&>
	void PayloadTarget([[maybe_unused]] const char* identifier, [[maybe_unused]] FunctionType onReceive)
	{
		internal::PayloadTargetClass<DataType, FunctionType>::Invoke(identifier, onReceive);
	}

	template<typename ContType>
		requires util::ConvertibleToCArray<ContType>&& std::is_same_v<typename ContType::value_type, const char*>
	Combo::Combo(const char* label, const ContType& data, const char* selectedValue, int* outSelectedIndex)
		: shouldCallEndCombo{ false }
	{
		// Find the index of the selected value.
		for (int i{}; static_cast<size_t>(i) < std::size(data); ++i)
			if (std::strcmp(data[i], selectedValue) == 0)
			{
				*outSelectedIndex = i;
				break;
			}
		// Draw the combo
		CallCombo(label, std::data(data), std::size(data), outSelectedIndex);
	}

	template<typename ContType>
		requires util::ConvertibleToCArray<ContType>&& std::is_same_v<typename ContType::value_type, const char*>
	Combo::Combo(const char* label, const ContType& data, int* selectedIndex)
		: shouldCallEndCombo{ false }
	{
		CallCombo(label, std::data(data), std::size(data), selectedIndex);
	}

	template<typename ContType>
		requires util::ConvertibleToCArray<ContType>&& std::is_same_v<typename ContType::value_type, float>
	void PlotLines([[maybe_unused]] const char* label, [[maybe_unused]] const ContType& cont, [[maybe_unused]] Vec2 graphSize, [[maybe_unused]] float scaleMin, [[maybe_unused]] float scaleMax, [[maybe_unused]] const char* overlayText)
	{
#ifdef IMGUI_ENABLED
		ImGui::PlotLines(label, std::data(cont), static_cast<int>(std::size(cont)), 0, overlayText, scaleMin, scaleMax, graphSize);
#endif
	}

}