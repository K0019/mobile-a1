#pragma once
#include "pch.h"
#include "BehaviourTree.h"
class BehaviorTreeSystem;

class BehaviorTreeSystem {
    friend class ST<BehaviorTreeSystem>;

public:
   void Initialize();
   void Shutdown(); 
   void UpdateAll(float dt);
   
   bool LoadAll(const std::string& dir);
   bool ReloadAll();
//   bool SaveAll(const std::string& dir);

   BehaviorTree* Get(const std::string& name) const; // get the tree itself NOT IN USE 
   bool Exists(const std::string& name) const; // check if it exist NOT IN USE

   bool CreateFromAsset(const BehaviorTreeAsset& asset); //helper function for loading
   void Remove(const std::string& name);  // not in use
   //std::vector<std::string> List() const;

    BehaviorTreeSystem() = default;
    ~BehaviorTreeSystem() = default;

    BehaviorTreeSystem(const BehaviorTreeSystem&) = delete;
    BehaviorTreeSystem& operator=(const BehaviorTreeSystem&) = delete;

private:

    std::map<std::string, std::unique_ptr<BehaviorTree>> trees; // store all the name and pointer of tree
    std::string defaultDir; // filepath to the folder where i store my tree
};

