/******************************************************************************/
/*!
\file   EventsQueue.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/IRegisteredComponent.h"
#include "UI/RectTransform.h"

class IUIComponentBase : public virtual ecs::IComponentCallbacks
{
};

template <typename T>
class IUIComponent : public IUIComponentBase
{
public:
	void OnAttached() override;
};

template <typename T>
void IUIComponent<T>::OnAttached()
{
	ecs::GetEntity(static_cast<T*>(this))->AddComp(RectTransformComponent{});
}

//class IUIComponentWithInput
//	: public IUIComponent
//{
//public:
//	void OnAttached() override;
//};
