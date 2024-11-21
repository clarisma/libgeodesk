#include <geodesk/geodesk.h>

using namespace geodesk;

int main()
{
    int64_t count = 0;
    int64_t localTagsCount = 0;
    Features world(R"(c:\geodesk\tests\w3.gol)");
	for(Feature f: world)
    {
        count++;
        if(f.ptr().tags().hasLocalKeys()) localTagsCount++;
    }
    printf("Out of %lld features, %lld have local-key tags.\n",
        count, localTagsCount);

    return 0;
}