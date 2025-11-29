/******************************************************************************/
/*!
\file   Delusion.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\author Ng Jia Cheng (50%)
\par    email: n.jiacheng\@digipen.edu
\par    DigiPen login: n.jiacheng

\brief
	Delusion component which stores a Delusion type and broadcasts a message that it
	has died if the Delusion goes below 0.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

#define DELUSION_TIER_ENUM \
X(F, "F") \
X(D, "D") \
X(C, "C") \
X(B, "B") \
X(A, "A") \
X(APLUS, "A+")

#define X(enumName, str) enumName,
enum class DELUSION_TIER
{
	DELUSION_TIER_ENUM
	TOTAL
};
GENERATE_ENUM_CLASS_ITERATION_OPERATORS(DELUSION_TIER)
#undef X

/*****************************************************************//*!
\class DelusionComponent
\brief
	To attach to entities which need to keep track of Delusion.
*//******************************************************************/
class DelusionComponent
	: public IRegisteredComponent<DelusionComponent>
	, public  IEditorComponent<DelusionComponent>
{
public:
	using DelusionType = float;


	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	DelusionComponent();

	/*****************************************************************//*!
		\brief
			Converts current Delusion to the DelusionTiers alphabet
		\return
			string containing alphabet (string for A+)
		*//******************************************************************/
	static const std::string& TierToString(DELUSION_TIER tier);

	/*****************************************************************//*!
	\brief
		Gets the current Delusion.
	\return
		The current Delusion.
	*//******************************************************************/
	DelusionType GetCurrDelusion() const;

	DELUSION_TIER GetCurrDelusionTier() const;

	/*****************************************************************//*!
	\brief
		Gets whether this entity is dead based on their current Delusion.
	\return
		True if the entity is dead. False otherwise.
	*//******************************************************************/
	bool IsUltReady() const;

	/*****************************************************************//*!
	\brief
		Gets the max Delusion.
	\return
		The max Delusion.
	*//******************************************************************/
	DelusionType GetMaxDelusion() const;

	/*****************************************************************//*!
	\brief
		Adds a certain amount of Delusion.
	\param amount
		The amount of Delusion to add.
	*//******************************************************************/
	void AddDelusion(DelusionType amount);

	/*****************************************************************//*!
	\brief
		Subtracts a certain amount of Delusion.
	\param amount
		The amount of Delusion to lose.
	*//******************************************************************/
	void LoseDelusion(DelusionType amount);

	/*****************************************************************//*!
	\brief
		Sets the current Delusion. Cannot exceed max Delusion.
	\param newAmount
		The amount of Delusion to set to.
	*//******************************************************************/
	void SetDelusion(DelusionType newValue);

	/*****************************************************************//*!
	\brief
		Sets a new max Delusion.
	\param newMaxAmount
		The new max Delusion.
	*//******************************************************************/
	void SetMaxDelusion(DelusionType newMaxAmount);

	/*****************************************************************//*!
	\brief
		Sets buffs and debuffs based on Delusion tier
	*//******************************************************************/
	void UpdateDelusionTier();

	DelusionType GetDelusionFraction();
private:
	/*****************************************************************//*!
	\brief
		Draws this component to the inspector window.
	\param comp
		The Delusion component to draw.
	*//******************************************************************/
	virtual void EditorDraw() override;

private:
	DelusionType maxDelusion;
	DelusionType currDelusion;
	DelusionType ultValue; //should be 0.9 of max delusion, so 90

	DelusionType gainRate;
	DelusionType lossRate;
	DELUSION_TIER prevTier;
	DELUSION_TIER currTier;
	static constexpr DelusionType defaultMax{ 100.0f };
	property_vtable()
};
property_begin(DelusionComponent)
{
	property_var(maxDelusion),
	property_var(gainRate),
	property_var(lossRate),
	property_var(ultValue),
	property_var(currDelusion),
}
property_vend_h(DelusionComponent)