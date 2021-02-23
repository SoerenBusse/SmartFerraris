#include "SignalDetector.h"
#include <iostream>
SignalDetector::SignalDetector(CircularVector *circularVector, int signalCountForDetection, int signalCountForRelease, std::function<void()> detectionCallback) : _circularVector{circularVector}, _signalCountForDetection{signalCountForDetection}, _signalCountForRelease{signalCountForRelease}, _detectionCallback{detectionCallback}
{
}

void SignalDetector::AddMeasurement(int value)
{
    bool isSignal = _circularVector->IsSignalElseInsert(value);

    // We need to reset our counter, when the signal changes.
    // This will prevent cases like: n n n s s n n n s to be deteced as a signal.
    if (_previousState != isSignal)
    {
        _noSignalCount = 0;
        _signalCount = 0;
    }

    _previousState = isSignal;

    if (isSignal == SIGNAL)
    {
        _signalCount++;
    }
    else
    {
        _noSignalCount++;
    }

    // When a signal occours three times after each other the state changes to this one and resets the other counter
    // this way to change to a new state it requires again 3 states. This will prevent flapping when only one measurement is above threashold
    if (_signalCount == _signalCountForDetection)
    {
        _noSignalCount = 0;
        _detectionCallback();
        _currentState = SIGNAL;
    }

    if (_noSignalCount == _signalCountForRelease)
    {
        _signalCount = 0;
        _currentState = NO_SIGNAL;
    }
}