#include "SignalDetector.h"
#include <iostream>

int main()
{
    // The Initialization Buffer
    std::vector<int> initValues{87, 80, 80, 84, 86, 82, 86, 85, 84, 82, 82, 82, 92, 86, 85, 80, 84, 81, 88, 89, 87, 84, 84, 84, 86, 90, 90, 85, 85, 81, 81, 89, 88, 91, 80, 82, 81, 88, 85, 88, 82, 89, 80, 85, 84, 91, 80, 88, 84, 87, 82, 87, 84, 89, 86, 89, 80, 84, 84, 86, 86, 89, 87, 88, 93, 89, 86, 84, 84, 81, 90, 91, 91, 90, 91, 94, 96, 88, 91, 88, 90, 91, 88, 93, 85, 89, 88, 85, 84, 87, 92, 90, 89, 89, 88, 89, 89, 89, 88, 89};
    std::vector<int> testingValues{87, 198, 200, 197, 200, 82, 86, 85, 200, 200, 200, 81, 81, 81, 82, 84, 200, 200, 82, 84, 200};

    CircularVector *cV = new CircularVector(100, 7);

    SignalDetector sD{cV, 3, 3, []() {
                          std::cout << "Found detection" << std::endl;
                      }};

    // Init Values
    for (int i : initValues)
    {
        sD.AddMeasurement(i);
    }

    // Test Values
    // Should find two signals
    for (int i : testingValues)
    {
        sD.AddMeasurement(i);
    }

    delete cV;
}