#pragma once
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <RecastDebugDraw.h>
#include <Recast.h>

class NavMesh
{
public:
	NavMesh();
	~NavMesh();

	void BuildNavMesh();

private:
	rcConfig config;
	rcContext context;
	dtNavMesh* navMesh;
	dtNavMeshQuery* navMeshQuery;
};