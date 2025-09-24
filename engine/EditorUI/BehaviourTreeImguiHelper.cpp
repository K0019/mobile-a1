#include "BehaviourTreeImguiHelper.h"


bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset& out)
{
    Deserializer r(path.c_str());
    if (!r.IsValid()) return false;
    return r.Deserialize(&out);
}


bool IsCompositeType(const std::string& typeName)
{
    auto* n = ST<BTFactory>::Get()->Create(typeName);
    if (!n) return false;
    bool isComp = (dynamic_cast<CompositeNode*>(n) != nullptr);
    delete n;
    return isComp;
}

bool IsDecoratorType(const std::string& typeName)
{
    auto* n = ST<BTFactory>::Get()->Create(typeName);
    if (!n) return false;
    bool isDeco = (dynamic_cast<Decorator*>(n) != nullptr);
    delete n;
    return isDeco;
}

std::vector<int> FindNodesAtLevel(const BehaviorTreeAsset& a, unsigned int level)
{
    std::vector<int> out;
    out.reserve(a.nodes.size());
    for (int i = 0; i < static_cast<int>(a.nodes.size()); ++i)
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
    auto ext = std::filesystem::path(s).extension().string();
    return (ext == ".json" || ext == ".bht");
}

 std::string EnsureAllowedExt(const std::string& s, const std::string& fallbackExt){
    if (HasAllowedExt(s)) return s;
    return s + fallbackExt;
}

// Windows-safe-ish filename scrub (keeps it simple)
 std::string SanitizeFilename(std::string s) {
    static const char* bad = "\\/:*?\"<>|";
    for (char& c : s) {
        if (std::strchr(bad, c) || (unsigned char)c < 32) c = '_';
    }
    // trim spaces
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    return s;
}


 // ---- SAVE ----
 bool SaveBTAssetToFile(const std::string& fullPath, const BehaviorTreeAsset& asset)
 {
     try {
         Serializer writer(fullPath.c_str());
         if (!writer.IsOpen()) {
             CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] cannot open: " << fullPath;
             return false;
         }

         // Let the property system write everything (including arrays) correctly
         asset.Serialize(writer);

         if (!writer.SaveAndClose()) {
             CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] SaveAndClose failed: " << fullPath;
             return false;
         }
         return true;
     }
     catch (const std::exception& e) {
         CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] exception: " << e.what();
         return false;
     }
 }

 bool HasJsonExt(const std::string& s) {
     std::filesystem::path p(s);
     auto ext = p.extension().string();
     // case-insensitive compare to ".json"
     if (ext.size() != 5) return false;
     return (ext[0] == '.' &&
         (ext[1] == 'j' || ext[1] == 'J') &&
         (ext[2] == 's' || ext[2] == 'S') &&
         (ext[3] == 'o' || ext[3] == 'O') &&
         (ext[4] == 'n' || ext[4] == 'N'));
 }

 std::string EnsureJsonExt(const std::string& s) {
     return HasJsonExt(s) ? s : (s + ".json");
 }
 bool CreateNewBTFile(const std::string& dir,
     const std::string& fileStem,
     std::string& outFullPath)
 {
     try {
         if (dir.empty() || fileStem.empty()) return false;

         const std::string stem = SanitizeFilename(fileStem);
         if (stem.empty()) return false;

         std::filesystem::path folder(dir);
         std::filesystem::path full = folder / EnsureJsonExt(stem);

         // Build a minimal tree: name is the file *stem*, one root at level 0.
         BehaviorTreeAsset asset;
         asset.name = stem;

         BTNodeDesc root;
         root.nodeType = "Sequence";   // or whatever you want as your default root
         root.nodeLevel = 0;

         asset.nodes.clear();
         asset.nodes.push_back(root);

         if (!SaveBTAssetToFile(full.string(), asset))
             return false;

         outFullPath = full.string();
         return true;
     }
     catch (...) {
         return false;
     }
 }




  bool ValidateLevelOrder(const BehaviorTreeAsset& a, std::string& check)
 {
     if (a.nodes.empty()) { check = "No nodes."; return false; }
     if (a.nodes.front().nodeLevel != 0) { check = "First node must have level 0."; return false; }

     unsigned prev = 0;
     for (size_t i = 1; i < a.nodes.size(); ++i) {
         unsigned lvl = a.nodes[i].nodeLevel;
         if (lvl > prev + 1) {
             check = "Invalid level jump at index " + std::to_string(i) +
                 " (prev=" + std::to_string(prev) + ", cur=" + std::to_string(lvl) + ")";
             return false;
         }
         prev = lvl;
     }
     return true;
 }

  bool DeleteBTFile(const std::string& dir, const std::vector<std::string>& files,int currentIndex, std::string& deletedPath)
  {
      if (currentIndex < 0 || currentIndex >= (int)files.size())
          return false;

      const std::filesystem::path full = std::filesystem::path(dir) / files[currentIndex];
      std::error_code ec;
      if (std::filesystem::remove(full, ec)) {
          deletedPath = full.string();
          return true;
      }
      return false;
  }


  void RefreshBTList(const std::string& dir,
      std::vector<std::string>& files,
      int& currentIndex,
      BehaviorTreeAsset& loadedAsset,
      bool& hasAsset,
      std::string& lastLoadedPath)
  {
      namespace fs = std::filesystem;

      files.clear();
      hasAsset = false;

      // Validate folder
      std::error_code ec;
      if (dir.empty() || !fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
          currentIndex = -1;
          return;
      }

      // Gather .json / .bht files
      for (const auto& e : fs::directory_iterator(dir, ec)) {
          if (ec) break;
          if (!e.is_regular_file(ec)) continue;
          const std::string ext = e.path().extension().string();
          if (ext == ".json" || ext == ".bht") {
              files.emplace_back(e.path().filename().string());
          }
      }

      // Sort for stable UI
      std::sort(files.begin(), files.end());

      // Fix selection
      if (files.empty()) {
          currentIndex = -1;
          return;
      }
      if (currentIndex < 0 || currentIndex >= static_cast<int>(files.size())) {
          currentIndex = 0;
      }

      // Auto-load the selected file
      const fs::path full = fs::path(dir) / files[currentIndex];
      lastLoadedPath = full.string();

      BehaviorTreeAsset tmp;
      if (LoadBTAssetFromFile(lastLoadedPath, tmp)) {
          loadedAsset = std::move(tmp);      // copy or move, both fine here
          hasAsset = true;
      }
      else {
          hasAsset = false;                  // caller can show an error popup if desired
      }
  }