#ifndef CIRCULAR_VECTOR_H
#define CIRCULAR_VECTOR_H

#include <vector>

class CircularVector
{
public:
    CircularVector(unsigned int vectorItemCount, int minimalThreashold, float signalZScore);

    bool IsSignalElseInsert(int value);
    void InsertItem(int value);

    int GetCurrentMean();
    float GetCurrentStddev();
    float GetCurrentZScore();
    int GetCurrentValue();
private:
    // Number of items in the circular vector
    unsigned int _vectorItemCount;
    int _minimalThreshold;
    float _signalZScore;

    std::vector<int> _circularVector;

    // Statistics
    int _currentMean = 0;
    float _currentStddev = 0;
    float _currentZScore = 0;
    int _currentValue = 0;

    int CalculateMean();
    float CalculateStddev(int mean);
};

#endif