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

    Features world(R"(c:\geodesk\tests\de.gol)");
    timer.stop("Opening GOL");

    Tile tile = Tile::fromColumnRowZoom(544,355,10);
    Box bounds = tile.bounds();
    Key name = world.key("name");
    Key highway = world.key("highway");
    Key building = world.key("building");

    std::cout << "Keys: " << name << ", " << highway << ", " << building << "\n";

    for(int run=1; run<=10; run++)
    {
        std::cout << "\nRun " << run << "\n";
        /*
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
        */

        timer.start();
        size_t count1 = 0;
        size_t count2 = 0;
        size_t count3 = 0;
        for (Feature f : world(bounds))
        {
            if (f.tags().hasTag("name")) count1++;
            if (f.tags().hasTag("highway")) count2++;
            if (f.tags().hasTag("building")) count3++;
        }
        timer.stop("Traditional tag lookup");
        std::cout << count1 << " named, "
            << count2 << " highways, "
            << count3 << " buildings\n";

        timer.start();
        count1 = 0;
        count2 = 0;
        count3 = 0;
        for (Feature f : world(bounds))
        {
            if (f.tags().hasTag(name)) count1++;
            if (f.tags().hasTag(highway)) count2++;
            if (f.tags().hasTag(building)) count3++;
        }
        timer.stop("Tag lookup via Key");
        std::cout << count1 << " named, "
            << count2 << " highways, "
            << count3 << " buildings\n";
    }
    return 0;
}