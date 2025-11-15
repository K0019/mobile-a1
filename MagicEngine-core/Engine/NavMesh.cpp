#include "NavMesh.h"
#include <cstring>
#include <vector>
#include "Physics/Physics.h"
#include "Physics/Collision.h"
#include "Physics/JoltPhysics.h"
#include "Editor/Containers/GUICollection.h"
#include "Utilities/GameTime.h"
#include "VFS/VFS.h"
#include "FilepathConstants.h"
#include "Engine/NavMeshAgent.h"

namespace navmesh 
{
	float const NavMeshSystem::CELL_SIZE = 0.3f;
	float const NavMeshSystem::CELL_HEIGHT = 0.2f;
	int const NavMeshSystem::TILE_SIZE = 256;
	int const NavMeshSystem::MAX_TILE = 128;
	int const NavMeshSystem::MAX_POLY = 32768;
	
	std::vector<TileDataBuffer> NavMeshData::LoadAllTileBuffers()
	{
		if (filePath == "" || !VFS::FileExists(filePath))
			return std::vector<TileDataBuffer>{};

		std::vector<TileDataBuffer> result{};

		//Read all the data from the file.
		std::vector<uint8_t> allFileData{};
		if (!VFS::ReadFile(filePath, allFileData))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't read navmesh data from file.";
			return std::vector<TileDataBuffer>{};
		}
		
		//Read the tile count.
		std::size_t readOffset{ sizeof(uint32_t) };
		uint32_t tileCount{};
		std::memcpy(&tileCount, allFileData.data(), readOffset);

		//Read navmesh data.
		for (std::size_t count{}; count < tileCount; ++count)
		{
			//Read the data size value.
			TileDataBuffer dataBuffer{ std::make_pair<int, unsigned char*>(0, nullptr) };
			uint32_t dataSize{};
			std::memcpy(&dataSize, allFileData.data() + readOffset, sizeof(uint32_t));
			readOffset += sizeof(uint32_t);
			dataBuffer.first = static_cast<int>(dataSize);

			//Allocate memory for the navmesh data.
			if (dataBuffer.first != 0)
				dataBuffer.second = (unsigned char*)dtAlloc(dataBuffer.first, DT_ALLOC_PERM);

			//Read the navmesh data.
			std::memcpy(dataBuffer.second, allFileData.data() + readOffset, static_cast<std::size_t>(dataBuffer.first));
			readOffset += dataBuffer.first;

			result.push_back(dataBuffer);
		}

		return result;
	}

	void NavMeshData::SaveAllTileBuffers(std::vector<TileDataBuffer>& allTileData, const std::string& navMeshName)
	{
		//if the filepath doesn't exist create a filepath.
		if (filePath == "" || !VFS::FileExists(filePath))
		{
			std::string directory{Filepaths::navMeshDataSave + "/" + navMeshName};
			std::string extension{ ".navmesh" };
			filePath = directory + extension;
			int count{ 1 };
			while (VFS::FileExists(filePath))
				filePath = directory + std::to_string(count++) + extension;
		}

		std::vector<uint8_t> dataBuffer{};

		//Insert the data count to the buffer.
		uint32_t dataCount{ static_cast<uint32_t>(allTileData.size()) };
		dataBuffer.insert(dataBuffer.end(), (uint8_t*)&dataCount, (uint8_t*)&dataCount + sizeof(uint32_t));

		//Write the data to the file.
		for (TileDataBuffer& tileData : allTileData)
		{
			//Insert the data size.
			uint32_t dataSize{ static_cast<uint32_t>(tileData.first) };
			dataBuffer.insert(dataBuffer.end(), (uint8_t*)&dataSize, (uint8_t*)&dataSize + sizeof(uint32_t));

			//Insert the data.
			dataBuffer.insert(dataBuffer.end(), tileData.second, tileData.second + dataSize);
		}

		//Write the entire data to the file.
		if (!VFS::WriteFile(filePath, dataBuffer))
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't save navmesh data to file.";
	}

	NavMeshSurfaceComp::NavMeshSurfaceComp()
		: config{}
		, navMeshData{}
		, tileRefs{}
		, navMeshName{ "navmesh" }
	{
		std::memset(&config, 0, sizeof(rcConfig));
		config.cs = NavMeshSystem::CELL_SIZE;
		config.ch = NavMeshSystem::CELL_HEIGHT;
		config.tileSize = NavMeshSystem::TILE_SIZE;
		config.walkableSlopeAngle = 45.f;
		config.walkableHeight = NavMeshAgentComp::WALKABLE_HEIGHT;
		config.walkableClimb = NavMeshAgentComp::WALKABLE_HEIGHT;
		config.walkableRadius = NavMeshAgentComp::WALKABLE_RADIUS;
		config.maxEdgeLen = 32;
		config.maxSimplificationError = 1.3f;
		config.minRegionArea = 1;
		config.mergeRegionArea = 0;
		config.maxVertsPerPoly = 6;
		config.detailSampleDist = 6.f;
		config.detailSampleMaxError = 1.f;
		config.bmin[0] = config.bmin[1] = config.bmin[2] = -50.f;
		config.bmax[0] = config.bmax[1] = config.bmax[2] = 50.f;
		config.borderSize = config.walkableRadius + 3;
		config.width = config.height = config.tileSize + 2 * config.borderSize;
	}

	void NavMeshSurfaceComp::OnAttached()
	{

	}

	void NavMeshSurfaceComp::OnDetached()
	{
		if (!ecs::GetSystem<NavMeshSystem>())
			return;

		dtStatus status{};
		dtNavMesh* navMesh{ ecs::GetSystem<NavMeshSystem>()->GetNavMesh() };
		for (dtTileRef tileRef : tileRefs)
		{
			status = navMesh->removeTile(tileRef, nullptr, nullptr);
			if (dtStatusFailed(status))
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Couldn't remove tile from the global navmesh.";
			}
		}
	}

	NavMeshData& NavMeshSurfaceComp::GetNavMeshData()
	{
		return navMeshData;
	}

	void NavMeshSurfaceComp::AddTileRef(const dtTileRef& tileRef)
	{
		tileRefs.push_back(tileRef);
	}

	void NavMeshSurfaceComp::BakeNavMeshData()
	{
		//rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);
		JPH::AABox bound{ JPH::Vec3{config.bmin[0], config.bmin[1], config.bmin[2]}, JPH::Vec3{config.bmax[0], config.bmax[1], config.bmax[2]} };

		//Collect the triangle mesh of the collider.
		std::vector<float> vertices{};
		std::vector<int> indices{};
		ST<physics::JoltPhysics>::Get()->CollectAllTriangles(bound, vertices, indices);

		std::vector<TileDataBuffer> allTileData{};

		float worldTileSize{ config.cs * static_cast<float>(config.tileSize) };
		//int tileMinX{ static_cast<int>(std::floor(config.bmin[0] / worldTileSize)) };
		//int tileMinZ{ static_cast<int>(std::floor(config.bmin[2] / worldTileSize)) };
		int tileMaxX{ static_cast<int>(std::floor((config.bmax[0] - config.bmin[0]) / worldTileSize)) };
		int tileMaxZ{ static_cast<int>(std::floor((config.bmax[2] - config.bmin[2]) / worldTileSize)) };

		float tileBMin[3];
		float tileBMax[3];

		for (int tileZ{}; tileZ <= tileMaxZ; ++tileZ)
			for (int tileX{}; tileX <= tileMaxX; ++tileX)
			{
				tileBMin[0] = config.bmin[0] + tileX * worldTileSize;
				tileBMin[1] = config.bmin[1];
				tileBMin[2] = config.bmin[2] + tileZ * worldTileSize;
				
				tileBMax[0] = config.bmin[0] + (tileX + 1) * worldTileSize;
				tileBMax[1] = config.bmax[1];
				tileBMax[2] = config.bmin[2] + (tileZ + 1) * worldTileSize;

				allTileData.push_back(BakeNavMeshTile(vertices, indices, tileBMin, tileBMax, tileX, tileZ));
			}

		navMeshData.SaveAllTileBuffers(allTileData, navMeshName);

		for (TileDataBuffer& tileData : allTileData)
			dtFree(tileData.second);
	}

	TileDataBuffer NavMeshSurfaceComp::BakeNavMeshTile(std::vector<float>& vertices, std::vector<int>& indices,
											 float* bmin, float* bmax, int xIndex, int zIndex)
	{
		//Initialize Build Config.
		rcContext context{};
		
		rcConfig tileConfig{ config };

		rcVcopy(tileConfig.bmin, bmin);
		rcVcopy(tileConfig.bmax, bmax);
		tileConfig.bmin[0] -= tileConfig.borderSize * tileConfig.cs;
		tileConfig.bmin[2] -= tileConfig.borderSize * tileConfig.cs;
		tileConfig.bmax[0] += tileConfig.borderSize * tileConfig.cs;
		tileConfig.bmax[2] += tileConfig.borderSize * tileConfig.cs;

		//Rasterize the polygon meshes.
		//Allocate the height field.
		rcHeightfield* heightField{ rcAllocHeightfield() };
		if (!heightField)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Height Field couldn't be allocated.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Create height field.
		if (!rcCreateHeightfield(&context, *heightField, tileConfig.width, tileConfig.height, tileConfig.bmin, tileConfig.bmax, tileConfig.cs, tileConfig.ch))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Height Field couldn't be created.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		int vertCount{ static_cast<int>(vertices.size()) };
		int triCount{ static_cast<int>(indices.size()) / 3 };
		std::vector<unsigned char> triAreaIDs(triCount);
		std::memset(triAreaIDs.data(), 0, triCount * sizeof(unsigned char));
		rcMarkWalkableTriangles(&context, tileConfig.walkableSlopeAngle, vertices.data(), vertCount, indices.data(), triCount, triAreaIDs.data());

		//Rastrize the triangle mesh.
		if (!rcRasterizeTriangles(&context, vertices.data(), vertCount, indices.data(), triAreaIDs.data(), triCount, *heightField))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Could not rasterize triangles.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Filter meshes to get accurate mesh that is walkable.
		rcFilterLowHangingWalkableObstacles(&context, tileConfig.walkableClimb, *heightField);
		//Filter the mesh for extruded edges
		rcFilterLedgeSpans(&context, tileConfig.walkableHeight, tileConfig.walkableClimb, *heightField);
		//Filter the mesh for low steps like stairs.
		rcFilterWalkableLowHeightSpans(&context, tileConfig.walkableHeight, *heightField);

		//Compact the height field so that it is easier to handle.
		rcCompactHeightfield* compactHF{ rcAllocCompactHeightfield() };
		if (!compactHF)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate compact height field.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}
		if (!rcBuildCompactHeightfield(&context, tileConfig.walkableHeight, tileConfig.walkableClimb, *heightField, *compactHF))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build compact height field.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Delete height field.
		rcFreeHeightField(heightField);

		//Erode walkable areas.
		if (!rcErodeWalkableArea(&context, tileConfig.walkableRadius, *compactHF))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't erode compact height field.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
		if (!rcBuildDistanceField(&context, *compactHF))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build distance field.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}
		if (!rcBuildRegions(&context, *compactHF, 0, tileConfig.minRegionArea, tileConfig.mergeRegionArea))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build watershed region.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Create Contours
		rcContourSet* contourSet{ rcAllocContourSet() };
		if (!contourSet)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate contour set.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}
		//Build contours to trace and simplify region contours.
		if (!rcBuildContours(&context, *compactHF, tileConfig.maxSimplificationError, tileConfig.maxEdgeLen, *contourSet))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build contour set.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Build Polygon Mesh
		rcPolyMesh* pMesh{ rcAllocPolyMesh() };
		if (!pMesh)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate polygon mesh detail memory.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}
		if (!rcBuildPolyMesh(&context, *contourSet, tileConfig.maxVertsPerPoly, *pMesh))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build poly mesh.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		//Build detail polygon mesh which allows to access approximate height on each polygon.
		rcPolyMeshDetail* pMeshDetail{ rcAllocPolyMeshDetail() };
		if (!pMeshDetail)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate memory for detail polygon mesh.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}
		if (!rcBuildPolyMeshDetail(&context, *pMesh, *compactHF, tileConfig.detailSampleDist, tileConfig.detailSampleMaxError, *pMeshDetail))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't build detail polygon mesh.";
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		rcFreeCompactHeightfield(compactHF);
		rcFreeContourSet(contourSet);

		unsigned char* data = 0;
		int size{};

		// Update poly flags from areas.
		for (int i = 0; i < pMesh->npolys; ++i)
		{
			if (pMesh->areas[i] == RC_WALKABLE_AREA)
				pMesh->flags[i] = PolyFlags::WALKABLE;
		}

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
		params.walkableHeight = static_cast<float>(tileConfig.walkableHeight) * tileConfig.ch;
		params.walkableRadius = static_cast<float>(tileConfig.walkableRadius) * tileConfig.cs;
		params.walkableClimb = static_cast<float>(tileConfig.walkableClimb) * tileConfig.ch;
		rcVcopy(params.bmin, pMesh->bmin);
		rcVcopy(params.bmax, pMesh->bmax);
		params.cs = tileConfig.cs;
		params.ch = tileConfig.ch;
		params.tileX = xIndex;
		params.tileY = zIndex;
		params.tileLayer = 0;
		params.buildBvTree = true;

		if (!dtCreateNavMeshData(&params, &data, &size))
		{
			rcFreePolyMesh(pMesh);
			rcFreePolyMeshDetail(pMeshDetail);
			return std::make_pair<int, unsigned char*>(0, nullptr);
		}

		rcFreePolyMesh(pMesh);
		rcFreePolyMeshDetail(pMeshDetail);
		return std::make_pair<int, unsigned char*>(std::forward<int>(size), std::forward<unsigned char*>(data));
	}

	void NavMeshSurfaceComp::EditorDraw()
	{
		gui::VarDefault("Agent Height", &config.walkableHeight);
		gui::VarDefault("Agent Radius", &config.walkableRadius);
		gui::VarDefault("Agent Max Climb", &config.walkableClimb);
		gui::VarDefault("Agent Max Slope", &config.walkableSlopeAngle);

		if (gui::Button clearButton{ "Clear" })
		{
			navMeshData.filePath = "";
		}
		gui::SameLine();
		if (gui::Button bakeButton{ "Bake" })
		{
			BakeNavMeshData();
		}
	}

	NavMeshSystem::NavMeshSystem()
		: navMesh{ nullptr }
	{
	}

	void NavMeshSystem::OnAdded()
	{
		navMesh = dtAllocNavMesh();
		if (!navMesh)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate memory for the navmesh.";
			return;
		}

		//Parameters to initialize the navmesh.
		dtNavMeshParams param{};
		param.orig[0] = param.orig[1] = param.orig[2] = -50.f;
		param.tileWidth = static_cast<float>(TILE_SIZE) * CELL_SIZE;
		param.tileHeight = static_cast<float>(TILE_SIZE) * CELL_SIZE;
		param.maxTiles = MAX_TILE;
		param.maxPolys = MAX_POLY;

		//Initialize the navmesh
		dtStatus status;
		status = navMesh->init(&param);
		if (dtStatusFailed(status))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't initialize the navmesh.";
			return;
		}

		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshSurfaceComp>() }, endIter{ ecs::GetCompsEnd<NavMeshSurfaceComp>() }; compIter != endIter; ++compIter)
		{
			std::vector<TileDataBuffer> const& allTileData{ compIter->GetNavMeshData().LoadAllTileBuffers() };
			if (allTileData.size() == 0)
				continue;

			for (TileDataBuffer const& tileData : allTileData)
			{
				if (!tileData.second || tileData.first == 0)
					continue;

				dtTileRef tileRef{};
				status = navMesh->addTile(tileData.second, tileData.first, DT_TILE_FREE_DATA, 0, &tileRef);
				if (dtStatusFailed(status))
				{
					CONSOLE_LOG(LEVEL_ERROR) << "Couldn't add a tile to the global navmesh.";
					return;
				}
				compIter->AddTileRef(tileRef);
			}
		}
	}

	void NavMeshSystem::OnRemoved()
	{
		dtFreeNavMesh(navMesh);
	}

	dtNavMesh* NavMeshSystem::GetNavMesh()
	{
		return navMesh;
	}
}
