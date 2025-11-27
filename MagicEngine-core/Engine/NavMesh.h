/******************************************************************************/
/*!
\file	NavMesh.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   16/11/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Declaration of system, components, and objects that are used for the
	navmesh.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <DetourCrowd.h>
#include <RecastDebugDraw.h>
#include <Recast.h>
#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Utilities/Serializer.h"

namespace navmesh
{
	using TileDataBuffer = std::pair<int, unsigned char*>;

	/*****************************************************************//*!
	\brief
		Flag set to each polygon in the navmesh.
	*//******************************************************************/
	enum class PolyFlags
	{
		WALKABLE = 1
	};

	/*****************************************************************//*!
	\brief
		Status of the path of the navmesh agent. 
	*//******************************************************************/
	enum class NavMeshPathStatus
	{
		PATH_COMPLETE,
		PATH_PARTIAL,
		PATH_INVALID
	};

	/*****************************************************************//*!
	\brief
		Parameters needed for baking the navmesh.
	*//******************************************************************/
	struct NavMeshParams
		: public ISerializeable
	{
		float radius;
		float height;
		float climb;
		float angle;

		property_vtable()
	};

	/*****************************************************************//*!
	\brief
		Data collected when finding the nearest navmesh point.
	*//******************************************************************/
	struct NavMeshHit
	{
		Vec3 position;
		dtPolyRef poly;
		float distance;
	};

	/*****************************************************************//*!
	\brief
		Path of the navmesh agent.
	*//******************************************************************/
	struct NavMeshPath
	{
		std::vector<Vec3> corners;
		NavMeshPathStatus status;
	};

	/*****************************************************************//*!
	\brief
		Data of the baked navmesh.
	*//******************************************************************/
	struct NavMeshData
		: public ISerializeable
	{
		std::string filePath;

		/*****************************************************************//*!
		\brief
			Load all the navmesh data in the asset file.
		\return
			Vector that contains all the navmesh data.
		*//******************************************************************/
		std::vector<TileDataBuffer> LoadAllTileBuffers();

		/*****************************************************************//*!
		\brief
			Save all the baked navmesh data.
		\param allTileData
			vector that contains all the tile data and it's size.
		\param navMeshName
			name of the navmesh baked.
		*//******************************************************************/
		void SaveAllTileBuffers(std::vector<TileDataBuffer>& allTileData, const std::string& navMeshName);
		property_vtable()
	};

	/*****************************************************************//*!
	\brief
		Component to bake a layer of navmesh.
	*//******************************************************************/
	class NavMeshSurfaceComp
		: public IRegisteredComponent<NavMeshSurfaceComp>
		, public IEditorComponent<NavMeshSurfaceComp>
		, public ecs::IComponentCallbacks
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		NavMeshSurfaceComp();

		/*****************************************************************//*!
		\brief
			Called when the component is attached.
		*//******************************************************************/
		void OnAttached() override;

		/*****************************************************************//*!
		\brief
			Called when the component is detached.
		*//******************************************************************/
		void OnDetached() override;

		/*****************************************************************//*!
		\brief
			Get the navmesh data of the component.
		*//******************************************************************/
		NavMeshData& GetNavMeshData();

		/*****************************************************************//*!
		\brief
			Store the tile refernce.
		\param tileRef
			tile reference to store.
		*//******************************************************************/
		void AddTileRef(const dtTileRef& tileRef);

		void Deserialize(Deserializer& reader) override;

	private:
		//Data baked by the component.
		NavMeshData navMeshData;
		//Tiles of the baked navmesh.
		std::vector<dtTileRef> tileRefs;
		//Name of the navmesh baked.
		std::string navMeshName;
		//Params for baking the navmesh.
		NavMeshParams agentParam;

		/*****************************************************************//*!
		\brief
			Bake all the tiles of the navmesh.
		*//******************************************************************/
		void BakeNavMeshData();

		/*****************************************************************//*!
		\brief
			Bake a tile of the navmesh.
		\param vertices
			List of input vertices of all the colliders.
		\param indices
			List of input indices of all the colliders.
		\param bmin
			Minimum boundary of the navmesh.
		\param bmax
			Maximum boundary of the navmesh.
		\param xIndex
			x index of the navmesh tile.
		\param zIndex
			z index of the navmesh tile.
		\return
			navmesh data baked.
		*//******************************************************************/
		TileDataBuffer BakeNavMeshTile(std::vector<float>& vertices, std::vector<int>& indices,
			float* bmin, float* bmax, int xIndex, int zIndex);

		virtual void EditorDraw() override;

	public:
		property_vtable()
	};

	class NavMeshSystem
		: public ecs::System<NavMeshSystem>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		NavMeshSystem();

		/*****************************************************************//*!
		\brief
			Called when the component is attached.
		*//******************************************************************/
		void OnAdded() override;

		/*****************************************************************//*!
		\brief
			Called when the component is detached.
		*//******************************************************************/
		void OnRemoved() override;

		/*****************************************************************//*!
		\brief
			Get the global navmesh.
		*//******************************************************************/
		dtNavMesh* GetNavMesh();

		static const float CELL_SIZE;
		static const float CELL_HEIGHT;
		static const int TILE_SIZE;
		static const int MAX_TILE;
		static const int MAX_POLY;

	private:
		//global navmesh.
		dtNavMesh* navMesh;
	};

	/*****************************************************************//*!
	\brief
		Get the nearest point on the navmesh.
	\param sourcePos
		starting position to check the nearest position.
	\param hit
		result of the nearest position.
	\param maxDistance
		maximum distance of the nearest position it can be.
	\param areaMask
		restriction of the point to pick. See PolyFlags
	\return
		true if the point is found, false if not.
	*//******************************************************************/
	bool SamplePosition(Vec3 sourcePos, NavMeshHit& hit, float maxDistance, unsigned int areaMask);
}
property_begin(navmesh::NavMeshParams)
{
	property_var(radius),
	property_var(height),
	property_var(climb),
	property_var(angle)
}
property_vend_h(navmesh::NavMeshParams)
property_begin(navmesh::NavMeshData)
{
	property_var(filePath)
}
property_vend_h(navmesh::NavMeshData)
property_begin(navmesh::NavMeshSurfaceComp)
{
	property_var(navMeshData),
	property_var(agentParam)
}
property_vend_h(navmesh::NavMeshSurfaceComp)