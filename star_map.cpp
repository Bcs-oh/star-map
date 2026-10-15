#include "splashkit.h"
#include "splashkit-arrays.h"
#include "star_map.h"
#include "gui.h"

int main()
{
    Star_map_data star_map{};

    star_map.load_cities("cities.csv");
    star_map.load_stars("star.csv");
    
    celestial_object *target = nullptr;

    // Start Graphical User Interface (GUI)
    start_gui(star_map, target);

    return 0;
}
