/******************************************************************************/
/*!
\file   Messaging.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/23/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
    This is an interface file for the messaging system which allows code to communicate
    with other systems without knowing about their existence.

    This was previously implemented by Chua Wen Shing Bryan but has been rewritten
    with the main purpose of allowing a more natural interface with the system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

/*
* Usage:
* To Subscribe,     Messaging::Subscribe(EventName, *insertFunctionHere*)
* To Unsubscribe,   Messaging::Unsubscribe(EventName, *insertFunctionHere*)
* To call,          Messaging::BroadcastAll(EventName, parameters...)
* 
* When broadcasting, function parameters must exactly match the broadcast parameters.
* i.e. If the function expects a std::string, the parameters must also be std::string and not const char* or otherwise.
*      To alleviate this, you may explicitly specify the template parameters in the broadcast call. e.g. Messaging::BroadcastAll<std::string>()
*/

#pragma once

namespace Messaging {

    namespace Internal {

        /*****************************************************************//*!
        \enum INVOKE_RESULT
        \brief
            The result of invoking a subscriber.
        *//******************************************************************/
        enum class INVOKE_RESULT
        {
            SUCCESS,
            FAILURE_PARAM_COUNT_MISMATCH,
            FAILURE_PARAM_TYPE_INCOMPATIBILITY
        };

        /*****************************************************************//*!
        \class Subscriber
        \brief
            A function subscribed to an event.
        *//******************************************************************/
        class Subscriber
        {
            using InvokeFuncSig = std::remove_pointer_t<INVOKE_RESULT(*)(std::initializer_list<std::any>)>;

        public:
            /*****************************************************************//*!
            \brief
                Constructor.
            \tparam Args
                The parameter types of the arguments of the callback function.
            \param callbackFunc
                The function to call when the event is fired.
            *//******************************************************************/
            template <typename ...Args>
            Subscriber(void(*callbackFunc)(Args...));

            /*****************************************************************//*!
            \brief
                Constructor with more customizability.
            \tparam Args
                The parameter types of the arguments of the callback function.
            \tparam AdditionalArgs
                The types of the additional parameters.
            \param callbackFunc
                The function to call when the event is fired.
            *//******************************************************************/
            template <typename ...Args>
            Subscriber(uint64_t identifier, const std::function<void(Args...)>& callbackFunc);

            /*****************************************************************//*!
            \brief
                Constructor.
            \param params
                The parameters of the event firing.
            *//******************************************************************/
            void Invoke(std::initializer_list<std::any> params) const;

            /*****************************************************************//*!
            \brief
                Checks if this subscriber's callback function is the same as the specified identifier.
            \param otherIdentifier
                The identifier to compare against.
            \return
                Whether the identifier is the same as this subscriber's identifier.
            *//******************************************************************/
            bool IsSameAs(uint64_t otherIdentifier) const;

        private:
            /*****************************************************************//*!
            \brief
                Generates a lambda function that binds to a callback function with additional
                fixed parameters and runtime arguments at event invocation.
            \tparam Args
                The parameter types of the callback function.
            \param callbackFunc
                The function to invoke when the event is invoked.
            \return
                The function that invokes the callback function with the appropriate parameters.
            *//******************************************************************/
            template <typename ...Args>
            static std::function<InvokeFuncSig> GenerateIntermediateFunc(const std::function<void(Args...)>& callbackFunc);

        private:
            //! An identifier to compare subscribers.
            uint64_t identifier;
            //! The function to call to invoke this subscriber.
            std::function<InvokeFuncSig> func;
            
        };

        /*****************************************************************//*!
        \class Event
        \brief
            An event storing subscribers listening for this event's firing.
        *//******************************************************************/
        class Event
        {
        public:
            /*****************************************************************//*!
            \brief
                Adds a callback function as a subscriber to this event.
            \param subscriber
                The subscriber to add to this event.
            *//******************************************************************/
            void AddSubscriber(Subscriber&& subscriber);

            /*****************************************************************//*!
            \brief
                Removes a callback function subscribed to this event.
            \param identifier
                The identifier of the function that is subscribed to this event.
            *//******************************************************************/
            void RemoveSubscriber(uint64_t identifier);

            /*****************************************************************//*!
            \brief
                Calls all subscribers with the specified parameters.
            \param params
                The parameters to invoke the subscribers with.
            *//******************************************************************/
            void BroadcastAll(std::initializer_list<std::any> params) const;

        private:
            //! The subscribers to this event
            std::vector<Subscriber> subscribers;
        };

        /*****************************************************************//*!
        \class EventsList
        \brief
            A central class keeping track of all events.
        *//******************************************************************/
        class EventsList
        {
        public:
            EventsList() = default;
            EventsList(EventsList&& other) noexcept = default;

            /*****************************************************************//*!
            \brief
                Copy constructor. This does not copy any event.
            \param copy
                The events list to copy.
            *//******************************************************************/
            EventsList(const EventsList&);

            /*****************************************************************//*!
            \brief
                Adds a callback function as a subscriber to an event.
            \tparam Args
                The parameter types of the callback function.
            \param subscriber
                The subscriber to add to the event.
            *//******************************************************************/
            void AddSubscriber(const std::string& eventName, Subscriber&& subscriber);

            /*****************************************************************//*!
            \brief
                Removes a callback function subscribed to an event.
            \param eventName
                The name of the event.
            \param identifier
                The identifier of the subscriber that is subscribed to the event.
            *//******************************************************************/
            void RemoveSubscriber(const std::string& eventName, uint64_t identifier);

            /*****************************************************************//*!
            \brief
                Calls all subscribers of an event with the specified parameters.
            \param eventName
                The name of the event.
            \param params
                The parameters to invoke the subscribers with.
            *//******************************************************************/
            void BroadcastAll(const std::string& eventName, std::initializer_list<std::any> params);

            /*****************************************************************//*!
            \brief
                Calls all subscribers of an event with the specified parameters.
            \tparam Args
                The types of the parameters.
            \param eventName
                The name of the event.
            \param params
                The parameters to invoke the subscribers with.
            *//******************************************************************/
            template <typename ...Args>
            void BroadcastAll(const std::string& eventName, const Args&... params);

        private:
            /*****************************************************************//*!
            \brief
                Gets the event object associated with a name. Creates it if it doesn't exist.
            \param eventName
                The name of the event.
            \return
                The event.
            *//******************************************************************/
            Event& GetEvent(const std::string& eventName);

            //! The events.
            std::unordered_map<std::string, Event> events;

        };

    }

    /*****************************************************************//*!
    \brief
        Subscribes a function to an event.
    \tparam Args
        The parameter types of the function.
    \param eventName
        The name of the event.
    \param func
        The function.
    *//******************************************************************/
    template <typename ...Args>
    void Subscribe(const std::string& eventName, void(*func)(Args...));

    /*****************************************************************//*!
    \brief
        Unsubscribes a function from an event.
    \tparam Args
        The parameter types of the function.
    \param eventName
        The name of the event.
    \param func
        The function.
    *//******************************************************************/
    template <typename ...Args>
    void Unsubscribe(const std::string& eventName, void(*func)(Args...));

    /*****************************************************************//*!
    \brief
        Invokes all subscribers of an event with the specified parameters.
    \tparam Args
        The parameter types of the function.
    \param eventName
        The name of the event.
    \param params
        The parameters to pass to the subscribers.
    *//******************************************************************/
    template <typename ...Args>
    void BroadcastAll(const std::string& eventName, const Args&... params);

    /*****************************************************************//*!
    \brief
        Cleans up memory used by the messaging system.
    *//******************************************************************/
    void Cleanup();

}

#include "Messaging.ipp"
