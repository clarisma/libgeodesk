#include <geodesk/geodesk.h>
#include <iostream>
#include <fstream>

using namespace geodesk;

/*
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
 */

/*
int main()
{
    int64_t count = 0;
    Features world(R"(c:\geodesk\tests\wxx2.gol)");
    Features rels = world("ar[geodesk:missing_members]");
	for(Relation r: rels)
    {
        count++;
        std::cout << r << " is missing " << r["geodesk:missing_members"] <<  "members.\n";
	    for(Feature m: r.members())
	    {
	        std::cout << "- Present: " << m << std::endl;
	    }
    }
    return 0;
}
*/

/*
int main()
{
    int64_t count = 0;
    Features world(R"(c:\geodesk\tests\wxx.gol)");
    Features dupes = world("n[geodesk:duplicate]");
    std::vector<std::string> nodes;
    for (auto dupe : dupes)
    {
        nodes.push_back(dupe.toString());
    }
    std::ranges::sort(nodes);
    std::ofstream file("c:\\geodesk\\tests\\wxx-dupes.txt");
    for (const auto& s : nodes)
    {
        file << s << '\n';
    }
    return 0;
}
*/

/*
int main()
{
    // -85.3510756, -160.0000000
    Box box = Box::ofWSEN(-160, -85.3510756, -160, -85.3510756);
    Features world(R"(c:\geodesk\tests\wxx.gol)");
    Features dupes = world("n[geodesk:duplicate]")(box);
    for (auto dupe : dupes)
    {
        std::cout << dupe << "\n";
    }
    return 0;
}
*/

int main()
{
    // -85.3510756, -160.0000000
    Features world(R"(c:\geodesk\tests\dexxu.gol)");
    Features dams = world("w[waterway=dam]");
    for (auto dam : dams)
    {
        std::vector<Node> nodes;
        dam.nodes().addTo(nodes);
        if(nodes.front() == nodes.back())
        {
            std::cout << dam << " has a closed ring.\n";
        }
        nodes.clear();
    }
    return 0;
}



