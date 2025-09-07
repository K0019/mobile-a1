#pragma once
#include "Filesystem.h"

//Helper function for search
bool MatchesFilter(const std::string& name);

struct BaseAssetCategory
{
	virtual const char* GetName() const = 0;
	virtual const char* GetIdentifier() const = 0;
	virtual void RenderBreadcrumb();
	virtual void Render() = 0;
};
