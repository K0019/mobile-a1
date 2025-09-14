/*
NOTE:

To do:
Serialisation (Read,Write) of the files
Figure out how to activate the AI (was thinking more of calling AI >typical major System like physics >etc...
really not sure how to begin coding or testing without c# tho need ask bossman
Blackboard
Integrate with ECS?
Figure out how to register nodes DYNAMICALLY (thinking of making a unique filetype for this but inside is just c#)

Decide if i want fixed decorator and composite nodes so user can only create leaf nodes


imgui:
create interface ( roughly up but all just static and fake)
add others for node and allow to choose which c# file to use
allow load and save of behavior tree files
Check if tree is valid (least priority)



*/



#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <iostream>
//class BehaviourTree;
//
//enum class NodeStatus
//{
//    N_READY, // node is enterable
//    N_RUNNING, // node is currently running
//    N_EXITING, // node has succeeded or failed
//    N_SUSPENDED // node won't exceute anything
//};
//
//enum class NodeResult
//{
//    N_IN_PROGRESS, // still being run 
//    N_SUCCESS, // node succeeded
//    N_FAILURE // node failed
//};
//
//struct TickContext {
//    float dt;                   // seconds since last tick
//    uint64_t frame;             // engine frame count
//    void* agent = nullptr;      // optional: pointer to your ECS entity or wrapper
//};


class BehaviourNode {

public:
	enum class Status { //    N_READY, // node is enterable
		N_RUNNING, // node is currently running
		N_SUCCESS, // node has succeeded or failed
		N_ERROR // node won't exceute anything
	};

	// A simple "context" type; customize as needed.
	using Context = std::unordered_map<std::string,
		std::variant<bool, int, float, double, std::string>>; // for blackboard

    explicit BehaviourNode(std::string name = "BehaviourNode")
        : name_(std::move(name)) {
    }

    virtual ~BehaviourNode() = default;

    // Default mirrors the GDScript: complain and return ERROR.
    virtual Status evaluate(const Context& ctx) {
        (void)ctx; // unused
        std::cerr << "Not implemented evaluate in BT, " << name_ << "\n";
        return Status::N_ERROR;
    }

    const std::string& name() const { return name_; }
    void set_name(std::string n) { name_ = std::move(n); }

private:
    std::string name_;

};


class RepeaterNode : public BehaviourNode {
public:
    explicit RepeaterNode(std::unique_ptr<BehaviourNode> child,
        std::string name = "RepeaterNode")
        : BehaviourNode(std::move(name)), child_(std::move(child)) {
    }

    Status evaluate(const Context& ctx) override {
        if (child_) {
            child_->evaluate(ctx);
        }
        // Always keep running, no matter what the child returned
        return Status::N_RUNNING;
    }

private:
    std::unique_ptr<BehaviourNode> child_;
};