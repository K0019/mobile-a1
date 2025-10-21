/******************************************************************************/
/*!
\file   GUIAsECS.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/12/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a collection of classes that implement a connection
  to ECS that editor windows can use for better organization.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Editor/Containers/GUICollection.h"
#include "ECS/ECSSysLayers.h"
#include "Utilities/Scheduler.h"

#pragma region Interface

namespace editor {

	/*****************************************************************//*!
	\class WindowBase
	\brief
		The base class of all ImGui windows.
	\tparam FinalType
		The type of the window.
	\tparam DuplicatesAllowed
		Whether multiple windows can exist at the same time.
	*//******************************************************************/
	template <typename FinalType, bool DuplicatesAllowed = true>
	class WindowBase : public gui::Window
	{
	public:
		//! Compile time constant that specifies whether a window type is allowed to be duplicated.
		inline static constexpr bool DUPLICATES_ALLOWED{ DuplicatesAllowed };

	protected:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param title
			The name of the window.
		\param initDimensions
			The initial width and height of the window.
		\param windowFlags
			The flags of the window.
		*//******************************************************************/
		WindowBase(const std::string& title, const gui::Vec2& initDimensions, gui::FLAG_WINDOW windowFlags = gui::FLAG_WINDOW::NONE);

		/*****************************************************************//*!
		\brief
			Encapsulates the drawing of the window with calls to switch the ECS
			pool to DEFAULT. Calls DrawWindow().
		*//******************************************************************/
		virtual void DrawContents() final;

		/*****************************************************************//*!
		\brief
			Draw the window. User implementations should override this with their
			gui code.
		*//******************************************************************/
		virtual void DrawWindow() = 0;

		/*****************************************************************//*!
		\brief
			Deletes the attached entity when the window is closed, either by
			the user or programatically.
		*//******************************************************************/
		virtual void OnOpenStateChanged() final;

		/*****************************************************************//*!
		\brief
			Called then the window is closed. User definitions may override
			this for behaviors upon closing the window.
		*//******************************************************************/
		virtual void UserOnOpenStateChanged();

	private:
		/*****************************************************************//*!
		\brief
			Schedules adding a system that will process this kind of component.
		*//******************************************************************/
		static bool RegisterWindowType();

		//! CRTP, execute per window type.
		inline static bool isRegistered{ RegisterWindowType() };
	};

	/*****************************************************************//*!
	\class WindowSystem
	\brief
		Draws all windows of a specific type. Automatically added to the
		EDITOR_GUI ecs pool by WindowBase::RegisterWindowType().
	\tparam WindowType
		The type of the window.
	*//******************************************************************/
	template <typename WindowType>
	class WindowSystem : public ::ecs::System<WindowSystem<WindowType>, WindowType>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		WindowSystem();

		/*****************************************************************//*!
		\brief
			Resets the ImGui id counter.
		*//******************************************************************/
		virtual bool PreRun() override;

	private:
		/*****************************************************************//*!
		\brief
			Draws one instance of the window.
		*//******************************************************************/
		void DrawWindow(WindowType& window);

	private:
		//! Tracks the ImGui id counter to ensure ID conflicts don't happen.
		int idCounter;
	};

	/*****************************************************************//*!
	\brief
		Creates a new instance of the specified window.
	\tparam WindowType
		The type of the window.
	*//******************************************************************/
	template <typename WindowType>
	void CreateGuiWindow();

}

#pragma endregion // Interface

#pragma region Definition

namespace editor {

	template<typename FinalType, bool DuplicatesAllowed>
	WindowBase<FinalType, DuplicatesAllowed>::WindowBase(const std::string& title, const gui::Vec2& initDimensions, gui::FLAG_WINDOW windowFlags)
		: Window{ title, initDimensions, windowFlags }
	{
		// gui::Window sets the window to unopen initially. We want it to be open whenever a ecs window is created.
		SetIsOpen(true);
	}

	template<typename FinalType, bool DuplicatesAllowed>
	void WindowBase<FinalType, DuplicatesAllowed>::DrawContents()
	{
		// User windows expect the current pool to be DEFAULT, so we'll need to switch over.
		// Be careful not to do anything that references EDITOR_GUI while we're on the DEFAULT pool.
		::ecs::POOL originalPool{ ::ecs::GetCurrentPoolId() };
		::ecs::SwitchToPool(::ecs::POOL::DEFAULT);
		DrawWindow();
		::ecs::SwitchToPool(originalPool);
	}

	template<typename FinalType, bool DuplicatesAllowed>
	void WindowBase<FinalType, DuplicatesAllowed>::OnOpenStateChanged()
	{
		if (!GetIsOpen())
		{
			UserOnOpenStateChanged();
			::ecs::DeleteEntity(::ecs::GetEntity(this));
		}
	}

	template<typename FinalType, bool DuplicatesAllowed>
	void WindowBase<FinalType, DuplicatesAllowed>::UserOnOpenStateChanged()
	{
	}

	template<typename FinalType, bool DuplicatesAllowed>
	bool WindowBase<FinalType, DuplicatesAllowed>::RegisterWindowType()
	{
		// Schedule, because ECS needs to be initialized first.
		ST<Scheduler>::Get()->Add([]() -> void {
			// Add a system that will process this window type into the EDITOR_GUI ecs pool.
			::ecs::POOL originalPool{ ::ecs::GetCurrentPoolId() };
			::ecs::SwitchToPool(::ecs::POOL::EDITOR_GUI);
			::ecs::AddSystem(ECS_LAYER::PRE_PHYSICS_0, WindowSystem<FinalType>{});
			::ecs::SwitchToPool(originalPool);
		});
		return true;
	}

	template<typename WindowType>
	WindowSystem<WindowType>::WindowSystem()
		// Not really sure why the explicit scope is required, why just System_Internal doesn't work...
		: ::ecs::internal::System_Internal<WindowSystem<WindowType>, WindowType>{ &WindowSystem<WindowType>::DrawWindow }
		, idCounter{}
	{
	}

	template<typename WindowType>
	bool WindowSystem<WindowType>::PreRun()
	{
		idCounter = 0;
		return true;
	}

	template<typename WindowType>
	void WindowSystem<WindowType>::DrawWindow(WindowType& window)
	{
		window.Draw(++idCounter);
	}

	template<typename WindowType>
	void CreateGuiWindow()
	{
		// Create an entity in EDITOR_GUI with the window attached as a component.
		::ecs::POOL originalPool{ ::ecs::GetCurrentPoolId() };
		::ecs::SwitchToPool(::ecs::POOL::EDITOR_GUI);

		// If duplicates are not allowed, don't do anything if there already exists a window of the requested type.
		if constexpr (!WindowType::DUPLICATES_ALLOWED)
			if (::ecs::GetCompsEnd<WindowType>() - ::ecs::GetCompsBegin<WindowType>() >= 1)
			{
				::ecs::SwitchToPool(originalPool);
				return;
			}

		::ecs::CreateEntity()->AddCompNow(WindowType{});
		::ecs::SwitchToPool(originalPool);
	}

}

#pragma endregion // Definition
