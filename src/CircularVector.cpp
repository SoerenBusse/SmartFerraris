#include "CircularVector.h"
#include <numeric>
#include <cmath>
#include <iostream>

using namespace std;

CircularVector::CircularVector(unsigned int vectorItemCount, int minimalThreshold, float signalZScore) : _vectorItemCount{vectorItemCount}, _minimalThreshold{minimalThreshold}, _signalZScore{signalZScore}
{
}

bool CircularVector::IsSignalElseInsert(int value)
{
    // Ignore zero entries completly
    if (value == 0)
    {
        return false;
    }

    // Just insert all values until the vector is filled with the maximum size
    // There might be an signal in there however the mean and stddev will fit "non-signal" level over time automatically
    if (_circularVector.size() < _vectorItemCount)
    {
        _circularVector.push_back(value);
        return false;
    }

    // Calculate the mean and standard derivation of all values in our circularVector
    int mean = CalculateMean();

    // If mean equals zero bad things will happen ;)
    if (mean == 0)
    {
        return false;
    }

    float stddev = CalculateStddev(mean);

    // The same goes for stddev
    if (stddev == 0)
    {
        return false;
    }

    // Calculate z-score
    float z = (value - mean) / stddev;

    // Save statistics
    _currentMean = mean;
    _currentStddev = stddev;
    _currentZScore = z;
    _currentValue = value;

    // Check if the value is greater then the minimal threashold
    if (value < mean + _minimalThreshold)
    {
        InsertItem(value);
        
        return false;
    }

    // Is out z-score higher then the required z-score for being a signal?
    // Then we're returning true and doesn't add the signal to our circular vector
    if (z > _signalZScore)
    {
        return true;
    }

    InsertItem(value);

    return false;
}

void CircularVector::InsertItem(int value)
{
    _circularVector.push_back(value);

    // If the circular vector is full we need to erase the first
    if (_circularVector.size() >= _vectorItemCount)
    {
        _circularVector.erase(_circularVector.begin());
    }
}

int CircularVector::CalculateMean()
{
    int sum = accumulate(_circularVector.begin(), _circularVector.end(), 0);
    return sum / _circularVector.size();
}

float CircularVector::CalculateStddev(int mean)
{
    int sum_x2 = 0;

    // Calculate the sum for x^2
    for (int value : _circularVector)
    {
        sum_x2 += (value * value);
    }

    // We're using a more simplified version, which may have some accuracy errors, however it is good enough
    return sqrt((sum_x2 / _circularVector.size()) - (mean * mean));
}

int CircularVector::GetCurrentMean()
{
    return _currentMean;
}

float CircularVector::GetCurrentStddev()
{
    return _currentStddev;
}

float CircularVector::GetCurrentZScore()
{
    return _currentZScore;
}

int CircularVector::GetCurrentValue()
{
    return _currentValue;
}