//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2020- Ceetron Solutions AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//##################################################################################################
#include "cafSignal.h"

namespace external
{
using namespace caf;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SignalEmitter::SignalEmitter()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SignalEmitter::~SignalEmitter()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SignalEmitter::addEmittedSignal( AbstractSignal* signalToAdd ) const
{
    m_signals.push_back( signalToAdd );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::list<AbstractSignal*>& SignalEmitter::emittedSignals() const
{
    return m_signals;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SignalObserver::SignalObserver()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SignalObserver::~SignalObserver()
{
    disconnectAllSignals();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::list<AbstractSignal*>& SignalObserver::observedSignals() const
{
    return m_signals;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SignalObserver::addObservedSignal( AbstractSignal* signalToObserve ) const
{
    m_signals.push_back( signalToObserve );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SignalObserver::removeObservedSignal( AbstractSignal* signalToRemove ) const
{
    m_signals.remove( signalToRemove );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SignalObserver::disconnectAllSignals()
{
    for (auto observedSignal : m_signals)
    {
        observedSignal->disconnect( const_cast<SignalObserver*>( this ) );
    }
}
} //namespace external
