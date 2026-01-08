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

//void BTFactory::SetAllFilePath()
//{
//    if (!VFS::FileExists(Filepaths::behaviourTreeSave))
//        return;
//
//    std::vector<std::string> filesInDir = VFS::ListDirectory(Filepaths::behaviourTreeSave);
//
//    for (const auto& filename : filesInDir)
//    {
//        std::string stem = VFS::GetStem(filename);
//
//        if (stem.empty()) //equivalent of !is_regular_file
//        {
//            continue;
//        }
//        filePaths[stem] = filename;
//    }
//}

static inline void trim_ascii_ws(std::string& s) {
    auto ws = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v'; };
    size_t i = 0; while (i < s.size() && ws((unsigned char)s[i])) ++i; s.erase(0, i);
    while (!s.empty() && ws((unsigned char)s.back())) s.pop_back();
}

static inline bool ends_with_json_ci_sanitized(const std::string& path) {
    // get filename
    size_t slash = path.find_last_of("/\\");
    const std::string fname = (slash == std::string::npos) ? path : path.substr(slash + 1);

    // last '.' must be after last slash
    size_t dot = fname.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= fname.size()) return false;

    // sanitize extension: keep only letters/digits, lowercased
    std::string ext;
    for (size_t i = dot + 1; i < fname.size(); ++i) {
        unsigned char c = (unsigned char)fname[i];
        if (std::isalnum(c)) ext.push_back((char)std::tolower(c));
    }
    return ext == "json";
}

void BTFactory::SetAllFilePath()
{
    CONSOLE_LOG(LEVEL_DEBUG) << "RUNNING SetAllFilePath()";
    filePaths.clear();

    const std::string dir = VFS::NormalizePath(Filepaths::behaviourTreeSave); // "behaviourtrees"
    auto entries = VFS::ListDirectory(dir);

    CONSOLE_LOG(LEVEL_DEBUG) << "[BT] ListDirectory('" << dir << "') -> " << entries.size() << " entries";
    for (auto& e : entries) CONSOLE_LOG(LEVEL_DEBUG) << "  [BT] entry: " << e;
    if (entries.empty()) return;

    for (std::string nameOrPath : entries) {   // copy; we'll sanitize
        trim_ascii_ws(nameOrPath);

        // canonicalize under dir
        const bool hasPrefix = nameOrPath.rfind(dir, 0) == 0;
        std::string fullPath = hasPrefix ? VFS::NormalizePath(nameOrPath)
            : VFS::JoinPath(dir, nameOrPath);
        trim_ascii_ws(fullPath);

        if (!ends_with_json_ci_sanitized(fullPath)) {
            CONSOLE_LOG(LEVEL_DEBUG) << "[BT] skip (not .json after sanitize): " << fullPath;

            continue;
        }

        // derive stem from filename
        size_t slash = fullPath.find_last_of("/\\");
        std::string fname = (slash == std::string::npos) ? fullPath : fullPath.substr(slash + 1);
        size_t dot = fname.find_last_of('.');
        std::string stem = (dot == std::string::npos) ? fname : fname.substr(0, dot);
        trim_ascii_ws(stem);
        if (stem.empty()) {
            CONSOLE_LOG(LEVEL_DEBUG) << "[BT] skip (empty stem): " << fullPath;
            continue;
        }

        filePaths[stem] = fullPath;   // e.g. "testclick" -> "behaviourtrees/testclick.json"
        CONSOLE_LOG(LEVEL_DEBUG) << "  [BT] mapped '" << stem << "' -> '" << fullPath << "'";
    }

    CONSOLE_LOG(LEVEL_DEBUG) << "[BT] filePaths.size=" << filePaths.size();

    // Optional: prove reads succeed once
    // for (auto &kv : filePaths) {
    //     std::string buf; bool ok = VFS::ReadFile(kv.second, buf);
    //     CONSOLE_LOG(LEVEL_DEBUG) << "[BT] probe read " << kv.second
    //                              << " ok=" << ok << " bytes=" << buf.size();
    // }
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