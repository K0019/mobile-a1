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

	enum PolyFlags
	{
		WALKABLE = 1
	};

	struct NavMeshHit
	{
		Vec3 position;
		dtPolyRef poly;
		float distance;
	};

	struct NavMeshData
		: public ISerializeable
	{
		std::string filePath;
		std::vector<TileDataBuffer> LoadAllTileBuffers();
		void SaveAllTileBuffers(std::vector<TileDataBuffer>& allTileData, const std::string& navMeshName);
		property_vtable()
	};

	class NavMeshSurfaceComp
		: public IRegisteredComponent<NavMeshSurfaceComp>
		, public IEditorComponent<NavMeshSurfaceComp>
		, public ecs::IComponentCallbacks
	{
	public:
		NavMeshSurfaceComp();

		void OnAttached() override;
		void OnDetached() override;

		NavMeshData& GetNavMeshData();
		void AddTileRef(const dtTileRef& tileRef);

	private:
		NavMeshData navMeshData;
		std::vector<dtTileRef> tileRefs;
		std::string navMeshName;

		void BakeNavMeshData();
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
		NavMeshSystem();

		void OnAdded() override;
		void OnRemoved() override;

		dtNavMesh* GetNavMesh();

		static const float CELL_SIZE;
		static const float CELL_HEIGHT;
		static const int TILE_SIZE;
		static const int MAX_TILE;
		static const int MAX_POLY;

	private:
		dtNavMesh* navMesh;
	};

	bool SamplePosition(Vec3 sourcePos, NavMeshHit& hit, float maxDistance, unsigned int areaMask);
}
property_begin(navmesh::NavMeshData)
{
	property_var(filePath)
}
property_vend_h(navmesh::NavMeshData)
property_begin(navmesh::NavMeshSurfaceComp)
{
	property_var(navMeshData)
}
property_vend_h(navmesh::NavMeshSurfaceComp)