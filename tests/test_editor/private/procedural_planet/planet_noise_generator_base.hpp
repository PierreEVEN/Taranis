#pragma once
#include <glm/vec3.hpp>

class PlanetData
{
public:
    virtual             ~PlanetData() = default;

    [[nodiscard]] float radius() const
    {
        return planet_radius;
    }

    struct WeatherData
    {
        // [0, 1] : humidity percentage level
        float humidity;
        // [-50, 50] celsius temp 
        float temperature;
    };
    virtual WeatherData get_weather_at_location(const glm::dvec3& location) = 0;

    struct TectonicData
    {
        // [-1, 0] is underwater, [0, 1] is land area
        float plate_layer;
        // [-1, 0] is rift, [0, 1] is mountains
        float mountain_layer;
        glm::vec3 test;
    };
    virtual TectonicData get_tectonic_plate_data_at_location(const glm::dvec3& location) = 0;

    struct RiverData
    {
        float river_width;
        float river_altitude;
        float distance_to_river;
    };
    virtual RiverData get_river_data_at_location(const glm::dvec3& location) = 0;

    struct BiomeDataAtLocation
    {
        // @TODO
        float biome_cursor;
    };
    virtual BiomeDataAtLocation get_biome_data_at_location(const glm::dvec3& location) = 0;


    virtual float get_height_at_location(const glm::dvec3& location) = 0;

  private:
    float planet_radius = 6000;
};