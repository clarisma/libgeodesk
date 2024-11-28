#include <iostream>
#include <chrono>
#include <geodesk/geodesk.h>

using namespace geodesk;

class Timer
{
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start_;
        std::cout << "Execution time: " << duration.count() << " seconds\n";
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};



int main()
{
    int64_t count;
    {
        Timer timer;
	    Features world(R"(c:\geodesk\tests\w3.gol)");
        Feature de = world("a[boundary=administrative][name:en=Germany]").one();
        count = world("r")(de).count();
    }
    printf("Found %lld features\n", count);

    return 0;
}