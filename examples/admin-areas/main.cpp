#include <geodesk/geodesk.h>

using namespace geodesk;

int main()
{
    Features world(R"(c:\geodesk\tests\w.gol)");
    Feature country = world("a[boundary=administrative][name=France][admin_level=2]").one();
    Features subAreas = world("a[boundary=administrative][name][admin_level]");
    int count = 0;
	for(Feature area: subAreas(country))
    {
        count++;
        std::cout << area["name"] << std::endl;
    }
    std::cout << count << " sub-areas" << std::endl;

    return 0;
}