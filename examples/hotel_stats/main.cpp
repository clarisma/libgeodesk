#include <geodesk/geodesk.h>

using namespace geodesk;

int main()
{
    Features world(R"(c:\geodesk\tests\w3.gol)");
    Features cities = world("a[boundary=administrative][admin_level=8]");
    Features hotels = world("na[tourism=hotel]");
    int64_t citiesCount = 0;
    int64_t citiesWithHotelsCount = 0;
	for(Feature city: cities)
    {
        citiesCount++;
        if(hotels.within(city)) citiesWithHotelsCount++;
    }
    printf("Out of %lld cities, %lld have at least one hotel.\n",
        citiesCount, citiesWithHotelsCount);

    return 0;
}