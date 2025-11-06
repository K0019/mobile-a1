#include "NavMesh.h"
#include <cstring>
#include <vector>
#include "ECS/ECS.h"
#include "Physics/Physics.h"
#include "Physics/Collision.h"
#include "Physics/JoltPhysics.h"

NavMesh::NavMesh()
	: context{ false }
	, navMesh{ nullptr }
	, navMeshQuery{ nullptr }
{
	navMeshQuery = dtAllocNavMeshQuery();
}

NavMesh::~NavMesh()
{
	if (navMesh)
		dtFreeNavMesh(navMesh);
	if (navMeshQuery)
		dtFreeNavMeshQuery(navMeshQuery);
}

void NavMesh::BuildNavMesh()
{
	//Initialize Build Config.
	std::memset(&config, 0, sizeof(rcConfig));
	config.cs = 0.25f;
	config.ch = 0.25f;
	config.walkableSlopeAngle = 45.f;
	config.walkableHeight = 4;
	config.walkableClimb = 4;
	config.walkableRadius = 4;
	config.maxEdgeLen = 32;
	config.maxSimplificationError = 1.3f;
	config.minRegionArea = 1;
	config.mergeRegionArea = 0;
	config.maxVertsPerPoly = 3;
	config.detailSampleDist = 6.f;
	config.detailSampleMaxError = 1.f;

	std::vector<float> minBound{ -1000.f, -1000.f, -1000.f };
	std::vector<float> maxBound{ 1000.f, 1000.f, 1000.f };

	rcVcopy(config.bmin, minBound.data());
	rcVcopy(config.bmax, maxBound.data());
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

	//Rasterize the polygon meshes.
	//Allocate the height field.
	rcHeightfield* heightField{ rcAllocHeightfield() };
	if (!heightField)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Height Field couldn't be allocated.";
		return;
	}

	//Create height field.
	if (!rcCreateHeightfield(&context, *heightField, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Height Field couldn't be created.";
		return;
	}

	unsigned char* triAreaIDs = new unsigned char[100];
	JPH::AABox bound{ JPH::Vec3{minBound[0], minBound[1], minBound[2]}, JPH::Vec3{maxBound[0], maxBound[1], maxBound[2]}};

	//Collect the triangle mesh of the collider.
	std::vector<float> vertices{};
	std::vector<int> indices{};
	ST<physics::JoltPhysics>::Get()->CollectAllTriangles(bound, vertices, indices);

	//Mark the triangle mesh if it's walkable based on the walkable slope angle.
	std::memset(triAreaIDs, 0, 100 * sizeof(unsigned char));
	int vertCount{ static_cast<int>(vertices.size()) };
	int triCount{ static_cast<int>(indices.size()) / 3 };
	rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, vertices.data(), vertCount, indices.data(), triCount, triAreaIDs);

	//Rastrize the triangle mesh.
	if (!rcRasterizeTriangles(&context, vertices.data(), vertCount, indices.data(), triAreaIDs, triCount, *heightField))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Could not rasterize triangles.";
		return;
	}

	delete[] triAreaIDs;

	//Filter meshes to get accurate mesh that is walkable.
	rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightField);
	//Filter the mesh for extruded edges
	rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightField);
	//Filter the mesh for low steps like stairs.
	rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightField);

	//Compact the height field so that it is easier to handle.
	rcCompactHeightfield* compactHF{ rcAllocCompactHeightfield() };
	if (!compactHF)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate compact height field.";
		return;
	}
	if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *heightField, *compactHF))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build compact height field.";
		return;
	}

	//Delete height field.
	rcFreeHeightField(heightField);

	//Erode walkable areas.
	if (!rcErodeWalkableArea(&context, config.walkableRadius, *compactHF))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't erode compact height field.";
		return;
	}

	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	if (!rcBuildDistanceField(&context, *compactHF))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build distance field.";
		return;
	}
	if (!rcBuildRegions(&context, *compactHF, 0, config.minRegionArea, config.mergeRegionArea))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build watershed region.";
		return;
	}

	//Create Contours
	rcContourSet* contourSet{ rcAllocContourSet() };
	if (!contourSet)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate contour set.";
		return;
	}
	//Build contours to trace and simplify region contours.
	if (!rcBuildContours(&context, *compactHF, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build contour set.";
		return;
	}

	//Build Polygon Mesh
	rcPolyMesh* pMesh{ rcAllocPolyMesh() };
	if (!pMesh)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate polygon mesh detail memory.";
		return;
	}
	if (!rcBuildPolyMesh(&context, *contourSet, config.maxVertsPerPoly, *pMesh))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build poly mesh.";
		return;
	}

	//Build detail polygon mesh which allows to access approximate height on each polygon.
	rcPolyMeshDetail* pMeshDetail{ rcAllocPolyMeshDetail() };
	if (!pMeshDetail)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate memory for detail polygon mesh.";
		return;
	}
	if (!rcBuildPolyMeshDetail(&context, *pMesh, *compactHF, config.detailSampleDist, config.detailSampleMaxError, *pMeshDetail))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build detail polygon mesh.";
		return;
	}

	rcFreeCompactHeightfield(compactHF);
	rcFreeContourSet(contourSet);

	unsigned char* navData = 0;
	int navDataSize = 0;

	dtNavMeshCreateParams params{};
	std::memset(&params, 0, sizeof(params));
	params.verts = pMesh->verts;
	params.vertCount = pMesh->nverts;
	params.polys = pMesh->polys;
	params.polyAreas = pMesh->areas;
	params.polyFlags = pMesh->flags;
	params.polyCount = pMesh->npolys;
	params.nvp = pMesh->nvp;
	params.detailMeshes = pMeshDetail->meshes;
	params.detailVerts = pMeshDetail->verts;
	params.detailVertsCount = pMeshDetail->nverts;
	params.detailTris = pMeshDetail->tris;
	params.detailTriCount = pMeshDetail->ntris;
	params.walkableHeight = config.height;
	params.walkableRadius = config.walkableRadius;
	params.walkableClimb = config.walkableClimb;
	rcVcopy(params.bmin, pMesh->bmin);
	rcVcopy(params.bmax, pMesh->bmax);
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true;

	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build NavMesh data.";
		return;
	}

	navMesh = dtAllocNavMesh();
	if (!navMesh)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate memory for the NavMesh.";
		return;
	}

	//Check if the detour navmesh can be initialized.
	dtStatus status{navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)};
	if (dtStatusFailed(status))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't initialize the Detour NavMesh.";
		return;
	}

	//Check if the detour navmesh query can be initialized.
	status = navMeshQuery->init(navMesh, 2048);
	if (dtStatusFailed(status))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Couldn't initialize the Detour NavMesh Query.";
		return;
	}

	// 1. Loop over every Tile in the mesh
	// (A non-tiled mesh will just have one tile at index 0)
	for (int i = 0; i < navMesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile{ navMesh->getTile(i) };
		if (!tile || !tile->header) 
			continue;

		// 2. Get the vertex array and count for this tile
		const float* tileVertices = tile->verts; // Array of (x, y, z) floats
		const int vertexCount = tile->header->vertCount;

		// 3. Loop through all vertices in this tile
		for (int j = 0; j < vertexCount; ++j)
		{
			// Get a pointer to the start of the j-th vertex
			const float* v = &tileVertices[j * 3];

			// v[0] is world-space X
			// v[1] is world-space Y
			// v[2] is world-space Z

			// You can now use these (e.g., add to a list)
			CONSOLE_LOG(LEVEL_DEBUG) << v[0] << "," << v[1] << "," << v[2];
		}
	}
}