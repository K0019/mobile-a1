/******************************************************************************/
/*!
\file   BehaviorTreeImguiHelper.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    This source file defines helper functions for the behavior tree imgui.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VFS/VFS.h"
#include "BehaviourTreeImguiHelper.h"
#include "BehaviourTreeFactory.h"
namespace bt {
    bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset* out)
    {
       // CONSOLE_LOG(LEVEL_DEBUG) << "The file path when LoadBTAssetFromFile is " << path;
        Deserializer r(path);
        if (!r.IsValid()) return false;
        return r.Deserialize(out);
    }


    bool IsCompositeType(const std::string& typeName)
    {
        auto* n{ ST<BTFactory>::Get()->Create(typeName) };
        if (!n) return false;
        bool isComp{ dynamic_cast<CompositeNode*>(n) != nullptr };
        delete n;
        return isComp;
    }

    bool IsDecoratorType(const std::string& typeName)
    {
        auto* n = ST<BTFactory>::Get()->Create(typeName);
        if (!n) return false;
        bool isDeco{ dynamic_cast<Decorator*>(n) != nullptr };
        delete n;
        return isDeco;
    }

    std::vector<int> FindNodesAtLevel(const BehaviorTreeAsset& a, unsigned int level)
    {
        std::vector<int> out;
        out.reserve(a.nodes.size());
        for (int i{ 0 }; i < static_cast<int>(a.nodes.size()); ++i)
            if (a.nodes[i].nodeLevel == level)
                out.push_back(i);
        return out;
    }

    std::string MakeNodeLabel(const BehaviorTreeAsset& a, int index)
    {
        if (index < 0 || index >= static_cast<int>(a.nodes.size()))
            return "<invalid>";
        return a.nodes[index].nodeType + "_" + std::to_string(index);
    }


    bool HasAllowedExt(const std::string& s) {
        auto ext{ std::filesystem::path(s).extension().string() };
        return (ext == ".json" || ext == ".bht");
    }

    std::string EnsureAllowedExt(const std::string& s, const std::string& fallbackExt)
    {
        if (HasAllowedExt(s)) return s;
        return s + fallbackExt;
    }

    // Windows-safe-ish filename scrub (keeps it simple)
    std::string SanitizeFilename(std::string s)
    {
        static const char* bad{ "\\/:*?\"<>|" };
        for (char& c : s)
            if (std::strchr(bad, c) || (unsigned char)c < 32) c = '_';

        // trim spaces
        while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
        while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
        return s;
    }


    // ---- SAVE ----
    bool SaveBTAssetToFile(const std::string& fullPath, const BehaviorTreeAsset& asset)
    {
        try
        {
            Serializer writer(fullPath);
            if (!writer.IsOpen())
            {
                CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] cannot open: " << fullPath;
                return false;
            }

            // Let the property system write everything (including arrays) correctly
            asset.Serialize(writer);

            if (!writer.SaveAndClose())
            {
                CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] SaveAndClose failed: " << fullPath;
                return false;
            }
            return true;
        }
        catch (const std::exception& e)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] exception: " << e.what();
            return false;
        }
    }

    bool HasJsonExt(const std::string& s)
    {
        std::filesystem::path p(s);
        auto ext{ p.extension().string() };
        // case-insensitive compare to ".json"
        if (ext.size() != 5) return false;
        return (ext[0] == '.' &&
            (ext[1] == 'j' || ext[1] == 'J') &&
            (ext[2] == 's' || ext[2] == 'S') &&
            (ext[3] == 'o' || ext[3] == 'O') &&
            (ext[4] == 'n' || ext[4] == 'N'));
    }

    std::string EnsureJsonExt(const std::string& s)
    {
        return HasJsonExt(s) ? s : (s + ".json");
    }
    bool CreateNewBTFile(const std::string& dir,
        const std::string& fileStem,
        std::string& outFullPath)
    {
        try {
            if (dir.empty() || fileStem.empty()) return false;

            const std::string stem{ SanitizeFilename(fileStem) };
            if (stem.empty()) return false;

            std::string fullPath = VFS::ConvertVirtualToPhysical(VFS::JoinPath(dir, EnsureJsonExt(stem)));

            // Build a minimal tree: name is the file *stem*, one root at level 0.
            BehaviorTreeAsset asset;
            asset.name = stem;

            BTNodeDesc root;
            root.nodeType = "Sequence";   // or whatever you want as your default root
            root.nodeLevel = 0;

            asset.nodes.clear();
            asset.nodes.push_back(root);

            if (!SaveBTAssetToFile(fullPath, asset))
                return false;

            outFullPath = fullPath;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }




    bool ValidateLevelOrder(const BehaviorTreeAsset& a, std::string& check)
    {
        if (a.nodes.empty()) { check = "No nodes."; return false; }
        if (a.nodes.front().nodeLevel != 0) { check = "First node must have level 0."; return false; }

        unsigned prev{ 0 };
        for (size_t i{ 1 }; i < a.nodes.size(); ++i) {
            unsigned lvl{ a.nodes[i].nodeLevel };
            if (lvl > prev + 1) {
                check = "Invalid level jump at index " + std::to_string(i) +
                    " (prev=" + std::to_string(prev) + ", cur=" + std::to_string(lvl) + ")";
                return false;
            }
            prev = lvl;
        }
        return true;
    }

    bool DeleteBTFile(const std::string& dir, const std::vector<std::string>& files, int currentIndex, std::string& deletedPath)
    {
        if (currentIndex < 0 || currentIndex >= (int)files.size())
            return false;

        const std::string virt = VFS::NormalizePath(VFS::JoinPath(dir, files[currentIndex]));

        if (VFS::DeleteFile(virt)) {
            deletedPath = virt;
            return true;
        }
        return false;
    }


    void RefreshBTList(const std::string& dir,
        std::vector<std::string>& files,
        int& currentIndex,
        BehaviorTreeAsset& loadedAsset,
        bool& hasAsset,
        [[maybe_unused]] std::string& lastLoadedPath)
    {
        files.clear();
        hasAsset = false;

        // Validate folder
        if (VFS::ListDirectory(dir).empty() || !VFS::FileExists(dir))   // TODO: VFS::IsDirectory()
        {
            currentIndex = -1;
            return;
        }

        // Gather .json / .bht files
        for (const auto& e : VFS::ListDirectory(dir))
        {
            //hack. TODOtoEnsureSameAsOriginalAuthor'sIntention: VFS::IsRegularFile()
            const std::string ext{ VFS::GetExtension(e)};
            if (ext == "") continue;
            if (ext == ".json" || ext == ".bht")
            {
                files.emplace_back(VFS::GetFilename(e));
            }
        }

        // Sort for stable UI
        std::sort(files.begin(), files.end());

        // Fix selection
        if (files.empty())
        {
            currentIndex = -1;
            return;
        }
        if (currentIndex < 0 || currentIndex >= static_cast<int>(files.size()))
        {
            currentIndex = 0;
        }

        const std::string full { VFS::JoinPath(dir, files[currentIndex])};
        hasAsset = LoadBTAssetFromFile(full, &loadedAsset);
    }


    bool LoadSelectedBT(const std::string& dir, const std::vector<std::string>& files, int currentIndex,
        BehaviorTreeAsset& out, bool& hasAsset, std::string& lastLoadedPath, int& selectedNodeIndex)
    {
        if (currentIndex < 0 || currentIndex >= (int)files.size()) {
            hasAsset = false;
            selectedNodeIndex = -1;
            return false;
        }

        //const std::string full{ (std::filesystem::path(dir) / files[currentIndex]).string() };
        const std::string full{ VFS::JoinPath(dir, files[currentIndex]) };

        if (!LoadBTAssetFromFile(full, &out))
        {
            hasAsset = false;
            lastLoadedPath = full;
            selectedNodeIndex = -1;
            return false;
        }

        hasAsset = true;
        lastLoadedPath = full;

        // default selection: first level-0 node if any
        if (!out.nodes.empty())
        {
            int idx = 0;
            for (int i = 0; i < (int)out.nodes.size(); ++i)
            {
                if (out.nodes[i].nodeLevel == 0) { idx = i; break; }
            }
            selectedNodeIndex = idx;
        }
        else
        {
            selectedNodeIndex = -1;
        }
        return true;
    }

    NODE_KIND ClassifyNodeType(const std::string& typeName)
    {
        BehaviorNode* n(ST<BTFactory>::Get()->Create(typeName));
        if (!n) return NODE_KIND::LEAF; // safest fallback

        if (dynamic_cast<CompositeNode*>(n))
        {
            delete n;
            return NODE_KIND::COMPOSITE;
        }
        if (dynamic_cast<Decorator*>(n))
        {
            delete n;
            return NODE_KIND::DECORATOR;
        }
        delete n;
        return NODE_KIND::LEAF;
    }

    void ListDirectChildren(const std::vector<BTNodeDesc>& nodes,
        int parentIdx,
        std::vector<int>& out)
    {
        out.clear();
        if (parentIdx < 0 || parentIdx >= (int)nodes.size()) return;

        const unsigned base{ nodes[parentIdx].nodeLevel };
        const unsigned want{ base + 1 };
        int i{ parentIdx + 1 };
        while (i < (int)nodes.size() && nodes[i].nodeLevel > base)
        {
            if (nodes[i].nodeLevel == want) out.push_back(i);
            ++i;
        }
    }

    int SubtreeEnd(const std::vector<BTNodeDesc>& nodes, int startIdx)
    {
        const unsigned base{ nodes[startIdx].nodeLevel };
        int i{ startIdx + 1 };
        while (i < (int)nodes.size() && nodes[i].nodeLevel > base) ++i;
        return i;
    }

    // Check if parent can add a child
    bool CanParentAddChild(const std::vector<BTNodeDesc>& nodes, int parentIdx)
    {
        if (parentIdx < 0 || parentIdx >= (int)nodes.size()) return false;

        const auto kind{ ClassifyNodeType(nodes[parentIdx].nodeType) };
        if (kind == NODE_KIND::LEAF) return false;
        if (kind == NODE_KIND::COMPOSITE) return true;

        // Decorator: allow only one direct child
        std::vector<int> kids;
        ListDirectChildren(nodes, parentIdx, kids);
        return kids.empty(); // can add if none yet
    }

    bool DeleteNodeAndSubtree(std::vector<BTNodeDesc>& nodes, int startIdx, int& selectedIndex)
    {
        if (startIdx < 0 || startIdx >= (int)nodes.size()) return false;

        const int end{ SubtreeEnd(nodes, startIdx) };

        //check current selection inside the removed block
        const bool selectionInside{ (selectedIndex >= startIdx && selectedIndex < end) };

        nodes.erase(nodes.begin() + startIdx, nodes.begin() + end);

        // Fix selection
        if (nodes.empty())
        {
            selectedIndex = -1; return true;
        }

        if (selectionInside)
        {
            // Prefer to select the prev sibling (startIdx-1), otherwise clamp to last
            selectedIndex = startIdx - 1;
            if (selectedIndex < 0) selectedIndex = 0;
            if (selectedIndex >= (int)nodes.size()) selectedIndex = (int)nodes.size() - 1;
        }
        else if (selectedIndex > (int)nodes.size() - 1)
        {
            selectedIndex = (int)nodes.size() - 1;
        }

        return true;
    }

    int CountRootNodes(const std::vector<BTNodeDesc>& nodes)
    {
        int c{ 0 };
        for (const auto& n : nodes) if (n.nodeLevel == 0) ++c;
        return c;
    }
}
