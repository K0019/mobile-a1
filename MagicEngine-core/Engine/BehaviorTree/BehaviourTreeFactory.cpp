/******************************************************************************/
/*!
\file   BehaviourTreeFactory.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Takumi Shibamoto (30%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\author Hong Tze Keat (70%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This file defines the runtime behavior of the BTFactory

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VFS/VFS.h"

#include "BehaviourTreeFactory.h"
#include "FilepathConstants.h"

BTFactory::BTFactory()
    : nodeTypes{}
{
    SetAllFilePath();
}

BTFactory::~BTFactory()
{
    nodeTypes.clear();
}

bool BTFactory::Register(const std::string& type, Maker fn) 
{
    return nodeTypes.emplace(type, fn).second;
}

BehaviorNode* BTFactory::Create(const std::string& type) const 
{
    auto it = nodeTypes.find(type);
    return it == nodeTypes.end() ? nullptr : it->second();
}

std::vector<std::string> BTFactory::RegisteredTypes() const 
{
    std::vector<std::string> out;
    for (auto& kv : nodeTypes) 
        out.push_back(kv.first);
    return out;
}

void BTFactory::SetAllFilePath()
{
    if (VFS::FileExists(Filepaths::virtualBehaviourTreeSave))
        return;

    std::vector<std::string> filesInDir = VFS::ListDirectory(Filepaths::virtualBehaviourTreeSave);

    for (const auto& filename : filesInDir)
    {
        std::string stem = VFS::GetStem(filename);

        if (stem.empty()) //equivalent of !is_regular_file
        {
            continue;
        }
        filePaths[stem] = filename;
    }
}

const std::string& BTFactory::GetFilePath(const std::string& btName) const
{
    return filePaths.contains(btName) ? filePaths.at(btName) : util::EmptyStr();
}

void BTFactory::GetAllBTNames(std::vector<std::string>* out) const
{
    for (const auto& pair : filePaths)
        out->push_back(pair.first);
    std::sort(out->begin(), out->end());
}

void BTFactory::Clear() 
{
    nodeTypes.clear();
}