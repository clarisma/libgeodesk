<img src="https://docs.geodesk.com/img/github-header.png">

GeoDesk is a fast and storage-efficient geospatial database for OpenStreetMap data. 
Also available [for Python](http://www.github.com/clarisma/geodesk-py) and [for Java](http://www.github.com/clarisma/geodesk).

## Why GeoDesk?

- **Small storage footprint** &mdash; GeoDesk's GOL files are only 20% to 50% larger than the original OSM data in PBF format &mdash; that's less than a tenth of the storage consumed by a traditional SQL-based database.

- **Fast queries** &mdash; typically 50 times faster than SQL. 

- **Fast to get started** &mdash; Converting `.osm.pbf` data to a GOL is 20 times faster than an import into an SQL database. Alternatively, download pre-made data tiles for just the regions you need and automatically assemble them into a GOL.

- **Intuitive API** &mdash; No need for object-relational mapping; GeoDesk queries return lightweight C++ objects. Quickly discover tags, way-nodes and relation members. Get a feature's geometry, measure its length/area. 
 
- **Proper handling of relations** &mdash; (Traditional geospatial databases deal with geometric shapes and require workarounds to support this unique and powerful aspect of OSM data.)

- **Optional integration with GEOS** for advanced geometric operations, such as buffer, union, simplify, convex and concave hulls, Voronoi diagrams, and much more.

- **Modest hardware requirements** &mdash; any 64-bit Windows, Linux or MacOS system will run GeoDesk.

- **Lightweight** &mdash; the full query engine adds less than 250 KB and has no link-time dependencies. 
  It runs entirely in-process, no database server required.
 
## Get Started

### Requirements

- [CMake](https://cmake.org/download) 3.14 or later
- C++20 compiler with a Standard Library for Windows, Linux or MacOS
- Java 16 or above (to run the [GOL Tool](https://docs.geodesk.com/gol))
 
### Build & Link

If your project uses **CMake**, add this to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(geodesk GIT_REPOSITORY 
    https://github.com/clarisma/libgeodesk.git)
FetchContent_MakeAvailable(geodesk)

target_link_libraries(my_program geodesk)
```

Alternatively, build GeoDesk explicitly:

```
git clone https://github.com/clarisma/libgeodesk.git
cd libgeodesk
mkdir build
cd build
cmake ..
cmake --build .
```

### Create a GOL

Create a Geographic Object Library based on any `.osm.pbf` file, using the 
[GOL Tool](https://www.geodesk.com/download) (Requires Java 16+).

For example:

```
gol build switzerland switzerland-latest.osm.pbf
```

### Example Application

Find all the pubs in Zurich (Switzerland) and print their names:

```cpp
#include <geodesk/geodesk.h>

using namespace geodesk;

int main(int argc, char* argv[])
{
    // Open switzerland.gol
    Features features("switzerland");      

    // Get the feature that represents the area of the city of Zurich
    Feature zurich = features(
        "a[boundary=administrative][admin_level=8]"
        "[name:en=Zurich]").one();

    // Define a set that contains nodes and areas that are pubs
    Features pubs = features("na[amenity=pub]");

    // Iterate through the pubs that are contained 
    // in the area of Zurich and print their names
    for (Feature pub: pubs.within(zurich)
    {
        std::cout << pub["name"] << std::endl;
    }
}
```

### More Examples

Find all movie theaters within 500 meters from a given point:

```cpp
Features movieTheaters = features("na[amenity=cinema]")
    .maxMetersFromLonLat(500, myLon, myLat);
```

*Remember, OSM uses British English for its terminology.*

Discover the bus routes that traverse a given street:

```cpp
for (Relation route: street.parents("[route=bus]"))
{
    std::cout << route["ref"] 
        << " from " << route["from"] 
        << " to " << route["to"] << std::endl;
}
```

Count the number of entrances of a building:

```cpp
int numberOfEntrances = building.nodes("[entrance]").count();
```

## Documentation

- [GeoDesk Developer's Guide](https://docs.geodesk.com/cpp)
- [API Reference](https://cppdoc.geodesk.com)

## Related Repositories

- [geodesk](http://www.github.com/clarisma/geodesk) &mdash; GeoDesk for Java
- [geodesk-py](http://www.github.com/clarisma/geodesk) &mdash; GeoDesk for Python
- [gol-tool](http://www.github.com/clarisma/gol-tool) &mdash; command-line utility for building, maintaining and querying GOL files

---

*OpenStreetMap is a trademark of the OpenStreetMap Foundation, and is used with their permission. GeoDesk is not endorsed by or affiliated with the OpenStreetMap Foundation.*
