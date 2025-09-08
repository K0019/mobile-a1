#pragma once
#include "GUICollection.h"

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
			if constexpr (EndFunc)
				if (isOpen || AlwaysCallEnd)
					EndFunc();
		}

		template<auto StartFunc, void (*EndFunc)(), bool AlwaysCallEnd, bool StartIsOptional>
		BeginEndBound<StartFunc, EndFunc, AlwaysCallEnd, StartIsOptional>::operator bool() const
		{
			return isOpen;
		}

	}

	template<typename ...Args>
	Tooltip::Tooltip(const char* fmt, const Args&... args)
		: internal::BeginEndBound_Tooltip{ false }
	{
#ifdef IMGUI_ENABLED
		ImGui::SetTooltip(fmt, args...);
#endif
	}

	template<size_t BufferSize>
	TextBoxWithBuffer<BufferSize>::TextBoxWithBuffer(const char* label)
		: label{ label }
		, buffer{}
	{
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
	void TextFormatted([[maybe_unused]] const char* fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		ImGui::Text(fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextColored([[maybe_unused]] const Vec4& color, [[maybe_unused]] const char* fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		ImGui::TextColored(color, fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextWrapped([[maybe_unused]] const char* fmt, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		ImGui::TextWrapped(fmt, args...);
#endif
	}

	template<typename ...Args>
	void TextDisabled([[maybe_unused]] const char* format, [[maybe_unused]] const Args&... args)
	{
#ifdef IMGUI_ENABLED
		ImGui::TextDisabled(format, args...);
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
			if (elemDrawFunc(v->at(i)))
				modified = true;
		}

		return modified;
#else
		return false;
#endif
	}

	template<typename DataType>
	void PayloadSource([[maybe_unused]] const char* identifier, [[maybe_unused]] const DataType& data, [[maybe_unused]] const char* dragLabel)
	{
#ifdef IMGUI_ENABLED
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(identifier, &data, sizeof(DataType));
			ImGui::Text("Dragging %s", dragLabel);
			ImGui::EndDragDropSource();
		}
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

}