#ifndef SIGNAL_DETECTOR_H
#define SIGNAL_DETECTOR_H

#include <functional>

#include "CircularVector.h"

#define SIGNAL true
#define NO_SIGNAL false

class SignalDetector
{

public:
    SignalDetector(CircularVector *circularVector, int signalCountForDetection, int signalCountForRelease, std::function<void()> detectionCallback);

    void AddMeasurement(int value);

private:
    CircularVector *_circularVector;
    int _signalCountForDetection;
    int _signalCountForRelease;
    std::function<void()> _detectionCallback;

    // The number of counted signals per state
    int _signalCount = 0;
    int _noSignalCount = 0;

    bool _currentState = NO_SIGNAL;
    bool _previousState = NO_SIGNAL;
};

#endif