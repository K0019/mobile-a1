#pragma once
#include "Managers/Filesystem.h"
#include "Editor/Containers/GUICollection.h"

namespace editor {

	struct BaseAssetCategory
	{
		virtual ~BaseAssetCategory() = default;
		virtual const char* GetName() const = 0;
		virtual const char* GetIdentifier() const = 0;
		virtual void RenderBreadcrumb();
		virtual void Render(const gui::TextBoxWithFilter& filter) = 0;
	};

}

