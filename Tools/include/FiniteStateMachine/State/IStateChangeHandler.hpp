#pragma once
#include <FiniteStateMachine/Helper/fsmconcepts.hpp>

template<typename TIndex>
requires StateIndex<TIndex>
class State;

template<typename TIndex>
class IStateChangeHandler {
public:
    virtual ~IStateChangeHandler() = default;
    virtual void on_enter(State<TIndex> &){}
    virtual void on_exit(State<TIndex> &){}
};
