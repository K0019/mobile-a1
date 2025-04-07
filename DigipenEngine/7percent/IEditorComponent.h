/******************************************************************************/
/*!
\file   IEditorComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for IEditorComponent, which registers a function for
  each type that inherits from it to draw a component to the ImGui window of Editor.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

namespace internal {
	/*****************************************************************//*!
	\class IEditorComponent
	\brief
		Provides an interface to invoke a function that draws the component
		to the ImGui inspector window.
	*//******************************************************************/
	class IEditorComponentBase
	{
	public:
		virtual void EditorDraw() = 0;
	};
}

#pragma region Interface

/*****************************************************************//*!
\class IEditorComponent
\brief
	Indicates that a component can be drawn to the ImGui inspector window.
\tparam CompType
	The type of the component.
*//******************************************************************/
template <typename CompType>
class IEditorComponent : public internal::IEditorComponentBase
{
private:
	/*****************************************************************//*!
	\brief
		Registers this type to ComponentDrawMethods.
	\return
		Dummy bool. True.
	*//******************************************************************/
	static bool RegisterType();

	//! CRTP to register each component type.
	static inline bool isRegistered{ RegisterType() };
};

namespace editor {

	/*****************************************************************//*!
	\class ComponentDrawMethods
	\brief
		This class contains the functions that draw components to the ImGui window for editor.
	*//******************************************************************/
	class ComponentDrawMethods
	{
	public:
		/*****************************************************************//*!
		\brief
			Registers a component type as supporting drawing to ImGui window for editor.
		\param compHash
			The hash of the component type.
		\param offset
			The byte offset from a component's base address such that IEditorComponent* works.
		*//******************************************************************/
		void Register(size_t compHash, size_t offset);

		/*****************************************************************//*!
		\brief
			Calls the function to draw the requested component type to the ImGui window for editor.
		\param compHash
			The hash of the component type.
		\param compHandle
			A handle to the component to be drawn to the ImGui window for editor.
		*//******************************************************************/
		bool Draw(size_t compHash, void* compHandle);

	private:
		//! Maps CompHash to ByteOffsets.
		std::unordered_map<size_t, size_t> byteOffsets;
	};

}

#pragma endregion // Interface

#pragma region Definition

template<typename CompType>
inline bool IEditorComponent<CompType>::RegisterType()
{
	ST<editor::ComponentDrawMethods>::Get()->Register(
		ecs::GetCompHash<CompType>(),
		// Multiple inheritance causes issues with the location of the vtable pointer for our specific base class.
		// We need to resolve this location offset ourselves. This util function helps us do this.
		util::ByteOffset<CompType, internal::IEditorComponentBase>()
	);
	return true;
}

#pragma endregion // Definition
