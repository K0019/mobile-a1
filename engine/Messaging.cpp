/******************************************************************************/
/*!
\file   Messaging.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/23/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
    This is the source file for the messaging system which allows code to communicate
    with other systems without knowing about their existence.

    This was previously implemented by Chua Wen Shing Bryan but has been rewritten
    with the main purpose of allowing a more natural interface with the system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Messaging.h"

namespace Messaging {

    namespace Internal {

        void Subscriber::Invoke(std::initializer_list<std::any> params) const
        {
            switch (func(params))
            {
            case INVOKE_RESULT::SUCCESS:
                return;
            case INVOKE_RESULT::FAILURE_PARAM_COUNT_MISMATCH:
                CONSOLE_LOG(LEVEL_ERROR) << "Messaging: A subscribed function did not match the number of parameters passed to an event!";
                return;
            case INVOKE_RESULT::FAILURE_PARAM_TYPE_INCOMPATIBILITY:
                CONSOLE_LOG(LEVEL_ERROR) << "Messaging: A subscribed function has parameter types not equivalent with the arguments passed to an event!";
                CONSOLE_LOG(LEVEL_ERROR) << "Try explicitly specifying the expected types when calling event broadcast. e.g. Messaging::BroadcastAll<std::string>()";
                return;
            }
        }

        bool Subscriber::IsSameAs(uint64_t toCompare) const
        {
            return identifier == toCompare;
        }

        void Event::AddSubscriber(Subscriber&& subscriber)
        {
            subscribers.emplace_back(std::forward<Subscriber>(subscriber));
        }

        void Event::RemoveSubscriber(uint64_t identifier)
        {
            for (auto iter{ subscribers.begin() }, end{ subscribers.end() }; iter != end; ++iter)
                if (iter->IsSameAs(identifier))
                {
                    subscribers.erase(iter);
                    return;
                }
        }

        void Event::BroadcastAll(std::initializer_list<std::any> params) const
        {
            for (const Subscriber& subscriber : subscribers)
                subscriber.Invoke(params);
        }

        EventsList::EventsList(const EventsList&)
            : events{}
        {
        }

        void EventsList::AddSubscriber(const std::string& eventName, Subscriber&& subscriber)
        {
            GetEvent(eventName).AddSubscriber(std::forward<Subscriber>(subscriber));
        }

        void EventsList::RemoveSubscriber(const std::string& eventName, uint64_t identifier)
        {
            GetEvent(eventName).RemoveSubscriber(identifier);
        }

        void EventsList::BroadcastAll(const std::string& eventName, std::initializer_list<std::any> params)
        {
            GetEvent(eventName).BroadcastAll(params);
        }

        Event& EventsList::GetEvent(const std::string& eventName)
        {
            auto eventIter{ events.find(eventName) };
            if (eventIter == events.end())
                return events.try_emplace(eventName).first->second;
            else
                return eventIter->second;
        }

    }

    void Cleanup()
    {
        ST<Internal::EventsList>::Destroy();
    }

}
