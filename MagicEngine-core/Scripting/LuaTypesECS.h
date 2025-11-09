/******************************************************************************/
/*!
\file   LuaScripting.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   11/01/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides an interface for lua scripting.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/ECS.h"

#define SCRIPT_REGISTER_COMP_GETTER(compType) \
	.addFunction("Get"#compType, [](ecs::EntityHandle entity) -> LuaWrapperComp_##compType { return entity->GetComp<compType>(); })


template <typename CompType>
class LuaWrapperComp
{
public:
	LuaWrapperComp(ecs::CompHandle<CompType> comp);

	ecs::CompHandle<CompType> operator->();
	operator ecs::CompHandle<CompType>();

	bool Valid() const;

protected:
	inline ecs::CompHandle<CompType> GetHandle() const;

	size_t compPtrRaw;
};

template<typename CompType>
LuaWrapperComp<CompType>::LuaWrapperComp(ecs::CompHandle<CompType> comp)
	: compPtrRaw{ reinterpret_cast<size_t>(comp) }
{
}

template<typename CompType>
ecs::CompHandle<CompType> LuaWrapperComp<CompType>::operator->()
{
	return GetHandle();
}

template<typename CompType>
bool LuaWrapperComp<CompType>::Valid() const
{
	return compPtrRaw;
}

template<typename CompType>
LuaWrapperComp<CompType>::operator ecs::CompHandle<CompType>()
{
	return GetHandle();
}

template <typename CompType>
inline ecs::CompHandle<CompType> LuaWrapperComp<CompType>::GetHandle() const
{
	return reinterpret_cast<ecs::CompHandle<CompType>>(compPtrRaw);
}

#define SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(compType) \
class LuaWrapperComp_##compType : public LuaWrapperComp<compType> { \
public: \
	LuaWrapperComp_##compType(ecs::CompHandle<compType> comp) \
		: LuaWrapperComp<compType>{ comp } {}

#define SCRIPT_GENERATE_PROPERTY_FUNCS(varType, Getter, Setter) \
	varType Getter() const { return GetHandle()->Getter(); } \
	void Setter(varType v) { GetHandle()->Setter(v); }

#define SCRIPT_GENERATE_COMP_WRAPPER_END() };


#define SCRIPT_REGISTER_COMP_BEGIN(compType) \
	.beginClass<LuaWrapperComp_##compType>(#compType) \
		.addFunction("Exists", &LuaWrapperComp_##compType::Valid)

#define SCRIPT_REGISTER_COMP_PROPERTY(compType, propertyName, Getter, Setter) \
		.addProperty(propertyName, &LuaWrapperComp_##compType::Getter, &LuaWrapperComp_##compType::Setter)

#define SCRIPT_REGISTER_COMP_END() .endClass()
