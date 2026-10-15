#include <geodesk/geodesk.h>

using namespace geodesk;

int main()
{
    Features world(R"(d:\geodesk\tests\w2.gol)");
    Feature country = world("a[boundary=administrative][name=France][admin_level=2]").one();
    Features routes = world("r[route=bus]");
    Features stops = world.withRole("stop");
    Features forwardBackward = world.ways().withRole("forward", "backward");
    for(Feature route : routes.within(country))
    {
        std::cout << route["name"] << std::endl;
        std::cout << "Stops:" << std::endl;
        for (Feature stop : stops.membersOf(route))
        {
            std::cout << "- " << stop["name"] << std::endl;
        }
        std::cout << "Forward/backward members:" << std::endl;
        for (Way way : forwardBackward.membersOf(route))
        {
            std::cout << "- " << way["name"] << " as " << way.role() << std::endl;
        }
    }

    return 0;
}