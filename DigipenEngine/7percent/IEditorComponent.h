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

#pragma region Interface

// This interface is for components that want to display variables and fields in editor.


// Place this in the constructor initialization list
// The draw function must have signature void (*)(CompType&)
// 
// e.g. AnimComp()
//			: REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw)
//			, ...
//		{ }
//

#define REGISTER_DRAW_FUNCTION_TO_EDITOR(CompDrawFunc) IEditorComponent{ IEditorComponent::EditorDrawFunc<CompDrawFunc>{} }

/*****************************************************************//*!
\class IEditorComponent
\brief
	For each class type inheriting from this class, registers a provided function that draws
	the component to the ImGui window for Editor.
\tparam CompType
	The type of the class inheriting this class.
\code{.cpp}
	class AnimComp : IEditorComponent<AnimComp>
\endcode
*//******************************************************************/
template <typename CompType>
class IEditorComponent
{
public:
	/*****************************************************************//*!
	\struct EditorDrawFunc
	\brief
		Dummy struct to allow compiler to deduce template constructor via argument type deduction.
	\tparam CompDrawFunc
		The function to be called to draw a component to the ImGui window for Editor.
	*//******************************************************************/
	template <void (*CompDrawFunc)(CompType&)>
	struct EditorDrawFunc { };

public:
	/*****************************************************************//*!
	\brief
		Constructs an instance of IEditorComponent, which registers the component type as a drawable to ImGui window for Editor.
	\tparam CompDrawFunc
		The function to be called to draw a component to the ImGui window for Editor.
	\param func
		The dummy struct used to work around the inability to specify template parameters for constructors of template classes.
	*//******************************************************************/
	template <void (*CompDrawFunc)(CompType&)>
	IEditorComponent(const EditorDrawFunc<CompDrawFunc>& func);

private:
	/*****************************************************************//*!
	\brief
		Registers a component type to ComponentDrawMethods.
	\tparam CompDrawFunc
		The function to be called to draw a component to the ImGui window for Editor.
	\return
		A dummy value to indicate that a component type has been registered.
	*//******************************************************************/
	template <void (*CompDrawFunc)(CompType&)>
	static bool RegisterComponent();

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
			Registers a function that draws a component type to the ImGui window for editor.
		\param compHash
			The hash of the component type.
		\param compDrawFunc
			The function that draws a component type to the ImGui window for editor.
		*//******************************************************************/
		static void Register(size_t compHash, void(*compDrawFunc)(void*));

		// Attempt to draw the component to the current active Imgui draw call.
		/*****************************************************************//*!
		\brief
			Calls the function to draw the requested component type to the ImGui window for editor.
		\param compHash
			The hash of the component type.
		\param compHandle
			A handle to the component to be drawn to the ImGui window for editor.
		*//******************************************************************/
		static bool Draw(size_t compHash, void* compHandle);

	private:
		//! Maps CompHash to DrawFunc.
		static std::unordered_map<size_t, void(*)(void*)> drawFuncs;
	};

}

#pragma endregion // Interface

#pragma region Definition

template<typename CompType>
template <void (*CompDrawFunc)(CompType&)>
IEditorComponent<CompType>::IEditorComponent(const EditorDrawFunc<CompDrawFunc>&)
{
	// Register only once
	static bool registered{ RegisterComponent<CompDrawFunc>() };
}

template<typename CompType>
template <void (*CompDrawFunc)(CompType&)>
bool IEditorComponent<CompType>::RegisterComponent()
{
	editor::ComponentDrawMethods::Register(
		typeid(CompType).hash_code(),
		[](void* compHandle) -> void { CompDrawFunc(*reinterpret_cast<CompType*>(compHandle)); }
	);
	return true;
}

#pragma endregion // Definition
