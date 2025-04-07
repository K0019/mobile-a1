/******************************************************************************/
/*!
\file   Transform.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for the Transform class that stores an entity's
  2D transform values.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "PercentMath.h"
#include <glm/glm.hpp>
#include <set>

// Forward declare for GetEntity()
namespace ecs {
	class Entity;
	using EntityHandle = Entity*;
	using ConstEntityHandle = const Entity*;
}

/*****************************************************************//*!
\class Transform
\brief
	Contains a position, rotation and scale value alongside support for parenting to another Transform,
	all of which is calculated into a 4x4 matrix for graphics use.
*//******************************************************************/
class Transform
{
private:
	// In case we want to switch dimensions in the future
	using Vec = Vector2;
	using Mat = Matrix3x3;

public:
	/*****************************************************************//*!
	\brief
		Default constructs a Transform.
	*//******************************************************************/
	Transform();

	/*****************************************************************//*!
	\brief
		Copy constructs a Transform.
		If the Transform is parented to another Transform, this Transform will also be parented to the same Transform.
		Children of the Transform are ignored.
	\param copy
		The transform to copy.
	*//******************************************************************/
	Transform(const Transform& copy);

	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~Transform();

	/*****************************************************************//*!
	\brief
		Copy assigns a Transform.
		If the Transform is parented to another Transform, this Transform will also be parented to the same Transform.
		Children of the Transform are ignored.
	\param copy
		The transform to copy.
	*//******************************************************************/
	Transform& operator=(const Transform& copy);

	/*****************************************************************//*!
	\brief
		Sets the local position of this Transform.
	\param newPos
		The new local position of the Transform.
	*//******************************************************************/
	void SetLocalPosition(const Vec& newPos);

	/*****************************************************************//*!
	\brief
		Adds a vector to the local position of this Transform.
	\param addPos
		The vector to be added to the local position of the Transform.
	*//******************************************************************/
	void AddLocalPosition(const Vec& addPos);

	/*****************************************************************//*!
	\brief
		Sets the world position of this Transform.
	\param newPos
		The new world position of the Transform.
	*//******************************************************************/
	void SetWorldPosition(const Vec& newPos);

	/*****************************************************************//*!
	\brief
		Adds a vector to the world position of this Transform.
	\param addPos
		The vector to be added to the world position of the Transform.
	*//******************************************************************/
	void AddWorldPosition(const Vec& addPos);

	/*****************************************************************//*!
	\brief
		Sets the z layer of this Transform.
	\param z
		The new z layer of the Transform.
	*//******************************************************************/
	void SetZPos(float z);

	/*****************************************************************//*!
	\brief
		Adds an amount to the z layer of this Transform.
	\param addZ
		The amount to be added to the z layer of the Transform.
	*//******************************************************************/
	void AddZPos(float addZ);

	/*****************************************************************//*!
	\brief
		Sets the local rotation of this Transform.
	\param newDegrees
		The new local rotation of the Transform.
	*//******************************************************************/
	void SetLocalRotation(float newDegrees);

	/*****************************************************************//*!
	\brief
		Adds an amount to the local rotation of this Transform.
	\param addDegrees
		The amount to be added to the local rotation of the Transform.
	*//******************************************************************/
	void AddLocalRotation(float addDegrees);

	/*****************************************************************//*!
	\brief
		Sets the world rotation of this Transform.
	\param newDegrees
		The new world rotation of the Transform.
	*//******************************************************************/
	void SetWorldRotation(float newDegrees);

	/*****************************************************************//*!
	\brief
		Adds an amount to the world rotation of this Transform.
	\param addDegrees
		The amount to be added to the world rotation of the Transform.
	*//******************************************************************/
	void AddWorldRotation(float addDegrees);

	/*****************************************************************//*!
	\brief
		Sets the local scale of this Transform.
	\param newScale
		The new local scale of the Transform.
	*//******************************************************************/
	void SetLocalScale(const Vec& newScale);

	/*****************************************************************//*!
	\brief
		Adds a vector to the local scale of this Transform.
	\param addScale
		The vector to be added to the local scale of the Transform.
	*//******************************************************************/
	void AddLocalScale(const Vec& addScale);

	/*****************************************************************//*!
	\brief
		Sets the world scale of this Transform.
	\param newScale
		The new world scale of the Transform.
	*//******************************************************************/
	void SetWorldScale(const Vec& newScale);

	/*****************************************************************//*!
	\brief
		Adds a vector to the world scale of this Transform.
	\param addScale
		The vector to be added to the world scale of the Transform.
	*//******************************************************************/
	void AddWorldScale(const Vec& addScale);


	/*****************************************************************//*!
	\brief
		Sets the local position, rotation and scale of this Transform.
	\param newPos
		The new local position.
	\param newScale
		The new local scale.
	\param newDegrees
		The new local rotation.
	*//******************************************************************/
	void SetLocal(const Vec& newPos = {}, const Vec& newScale = { 1.0f, 1.0f }, float newDegrees = 0.0f);

	/*****************************************************************//*!
	\brief
		Sets the local position, rotation, scale and z position of this Transform.
	\param z
		The new z position.
	\param newPos
		The new local position.
	\param newScale
		The new local scale.
	\param newDegrees
		The new local rotation.
	*//******************************************************************/
	void SetLocal(float z, const Vec& newPos = {}, const Vec& newScale = { 1.0f, 1.0f }, float newDegrees = 0.0f);

	/*****************************************************************//*!
	\brief
		Sets the local position, rotation, scale and z position of this Transform
		to the local values of another transform.
	\param transform
		The transform to copy local values from.
	*//******************************************************************/
	void SetLocal(const Transform& transform);

	/*****************************************************************//*!
	\brief
		Sets the world position, rotation and scale of this Transform.
	\param newPos
		The new world position.
	\param newScale
		The new world scale.
	\param newDegrees
		The new world rotation.
	*//******************************************************************/
	void SetWorld(const Vec& newPos = {}, const Vec& newScale = { 1.0f, 1.0f }, float newDegrees = 0.0f);

	/*****************************************************************//*!
	\brief
		Sets the world position, rotation, scale and z position of this Transform.
	\param z
		The new z position.
	\param newPos
		The new world position.
	\param newScale
		The new world scale.
	\param newDegrees
		The new world rotation.
	*//******************************************************************/
	void SetWorld(float z, const Vec& newPos = {}, const Vec& newScale = { 1.0f, 1.0f }, float newDegrees = 0.0f);

	/*****************************************************************//*!
	\brief
		Sets the world position, rotation, scale and z position of this Transform
		to the world values of another transform.
	\param transform
		The transform to copy world values from.
	*//******************************************************************/
	void SetWorld(const Transform& transform);

	/*****************************************************************//*!
	\brief
		Gets the local position of this Transform.
	\return
		The local position of the Transform.
	*//******************************************************************/
	const Vec& GetLocalPosition() const;

	/*****************************************************************//*!
	\brief
		Gets the local rotation of this Transform.
	\return
		The local rotation of the Transform.
	*//******************************************************************/
	float GetLocalRotation() const;

	/*****************************************************************//*!
	\brief
		Gets the local scale of this Transform.
	\return
		The local scale of the Transform.
	*//******************************************************************/
	const Vec& GetLocalScale() const;

	// These values are not cached. If performance is an issue due to too many recursion calls to parents, could look to cache these.
	/*****************************************************************//*!
	\brief
		Gets the world position of this Transform.
	\return
		The world position of the Transform.
	*//******************************************************************/
	Vec GetWorldPosition() const;

	/*****************************************************************//*!
	\brief
		Gets the world rotation of this Transform.
	\return
		The world rotation of the Transform.
	*//******************************************************************/
	float GetWorldRotation() const;

	/*****************************************************************//*!
	\brief
		Gets the world scale of this Transform.
	\return
		The world scale of the Transform.
	*//******************************************************************/
	Vec GetWorldScale() const;

	/*****************************************************************//*!
	\brief
		Gets the z position of this Transform.
	\return
		The z position of the Transform.
	*//******************************************************************/
	float GetZPos() const;


	/*****************************************************************//*!
	\brief
		Sets the parent of this transform. Changes to the parent transform will affect the world transform of this transform.
	\param parentTransform
		The transform to parent to.
	*//******************************************************************/
	void SetParent(Transform& parentTransform);

	/*****************************************************************//*!
	\brief
		Sets the parent of this transform. Changes to the parent transform will affect the world transform of this transform.
	\param parentTransform
		The transform to parent to.
	*//******************************************************************/
	void SetParent(Transform* parentTransform);

	/*****************************************************************//*!
	\brief
		Gets the parent of this transform.
	\return
		The parent transform.
	*//******************************************************************/
	Transform* GetParent();

	/*****************************************************************//*!
	\brief
		Gets the parent of this transform.
	\return
		The parent transform.
	*//******************************************************************/
	const Transform* GetParent() const;

	/*****************************************************************//*!
	\brief
		Gets the list of children attached to this transform.
	\return
		The list of children.
	*//******************************************************************/
	std::set<Transform*>& GetChildren();

	/*****************************************************************//*!
	\brief
		Gets the list of children attached to this transform.
	\return
		The list of children.
	*//******************************************************************/
	const std::set<Transform*>& GetChildren() const;

	/*****************************************************************//*!
	\brief
		Recursively gets the list of children attached to this transform.
	\return
		The list of children.
	*//******************************************************************/
	std::set<Transform*> GetChildrenRecursive();

	/*****************************************************************//*!
	\brief
		Gets the entity that holds this transform.
	\return
		The entity that holds this transform.
	*//******************************************************************/
	ecs::EntityHandle GetEntity();

	/*****************************************************************//*!
	\brief
		Gets the entity that holds this transform.
	\return
		The entity that holds this transform.
	*//******************************************************************/
	ecs::ConstEntityHandle GetEntity() const;

	/*****************************************************************//*!
	\brief
		Sets a glm::mat4 to this transform's world matrix.
	\param outMat4
		The glm::mat4 that will be set.
	*//******************************************************************/
	void SetMat4ToWorld(glm::mat4* outMat4) const;

	/*****************************************************************//*!
	\brief
		Draws this transform to the current ImGui window.
	*//******************************************************************/
	void EditorDraw();

private:
	/*****************************************************************//*!
	\brief
		Sets the state of this Transform and all children Transforms' internal matrix as dirty, so they need to be recalculated the next time they are accessed.
	*//******************************************************************/
	void SetDirty();

	/*****************************************************************//*!
	\brief
		Gets this transform's world matrix.
	\return
		The transform's world matrix.
	*//******************************************************************/
	const Mat& GetWorldMat() const;

	/*****************************************************************//*!
	\brief
		Sets the parent of this transform. Changes to the parent transform will affect the world transform of this transform.
	\param parentTransform
		The transform to parent to.
	\param informParent
		Whether to inform the previous parent that we're unparenting from it.
	*//******************************************************************/
	void SetParent(Transform* parentTransform, bool informOldParent);

private:
	//! The position of this Transform.
	Vec position;
	//! The z position of this Transform.
	float posZ; // For layering
	//! The rotation of this Transform.
	float rotation;
	//! The scale of this Transform.
	Vec scale;

	//! A flag to indicate whether the matrix within this Transform is dirty and thus needs to be recalculated before being accessed.
	mutable bool isTransformDirty;
	//! The matrix of this Transform.
	mutable Mat mat;

	//! A pointer to this Transform's parent.
	Transform* parent;
	//! The Transforms that are parented to this Transform.
	std::set<Transform*> children;
};
