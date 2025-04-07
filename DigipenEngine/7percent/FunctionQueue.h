/* DEPRECATED - PLANNED TO BE MERGED WITH "Scheduler" */


/******************************************************************************/
/*!
\file   FunctionQueue.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class that queues functions to be executed later.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

class FunctionQueue
{
    public:

    static void QueueOperation(std::function<void()>&& operation) {
        GetQueue().push(std::move(operation));
    }

    // Static method to execute all queued operations
    static void ExecuteQueuedOperations() {
        auto& queue = GetQueue();
        while(!queue.empty()) {
            queue.front()();
            queue.pop();
        }
    }

    private:
    // Private method to get the queue singleton
    static std::queue<std::function<void()>>& GetQueue() {
        static std::queue<std::function<void()>> queue;
        return queue;
    }
};


