#include <iostream>
#include <chrono>
#include <geodesk/geodesk.h>

using namespace geodesk;

class Timer
{
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    void start()
    {
        start_ = std::chrono::high_resolution_clock::now();
    }

    void stop(const char* msg)
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start_;
        std::cout <<msg << " took " << duration.count() << " seconds\n";
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};



int main()
{
    std::cout << "Timing v2\n";

    Timer timer;
    Features world(R"(c:\geodesk\tests\wxx2.gol)");
    timer.stop("Opening GOL");

    for(int run=1; run<=10; run++)
    {
        std::cout << "Run " << run << "\n";
        timer.start();
        Feature de = world("a[boundary=administrative][name:en=Germany]").one();
        timer.stop("Fetching area feature");

        timer.start();
        Features testSet = world("w")(de);
        timer.stop("Creating filtered set");

        int64_t count;
        timer.start();
        count = testSet.count();
        timer.stop("Query");

        printf("v2: Found %lld ways in Germany\n", count);
    }
    return 0;
}