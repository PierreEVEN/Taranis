#pragma once
#include "planet_map.hpp"
#include "planet_noise_generator_base.hpp"

#include <memory>
#include <third_party/fastnoise/FastNoise.h>

class PrecomputedPlanetData : public PlanetData
{
public:
    PrecomputedPlanetData();

    WeatherData get_weather_at_location(const glm::dvec3& location) override;

    TectonicData get_tectonic_plate_data_at_location(const glm::dvec3& location) override;

    RiverData get_river_data_at_location(const glm::dvec3& location) override;

    BiomeDataAtLocation get_biome_data_at_location(const glm::dvec3& location) override;

    float get_height_at_location(const glm::dvec3& location) override;

private:
    FastNoise noise;
    FastNoise noise2;
    FastNoise noise3;

    struct PlanetPixel
    {
        double test_val;
    };

    PlanetMap<PlanetPixel> planet_map;

};