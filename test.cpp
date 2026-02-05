#include "Event.h"
#include <iostream>

int main(){
    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
    using Duration = std::chrono::seconds;
    using String = std::string;

    TimePoint adesso = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now());
    Duration durata = Duration(3600); // 1 hour

    Event meeting("Team Meeting", adesso, durata);

    std::cout << meeting;

    return 0;
}
