#include "precomputed_planet_data.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

PrecomputedPlanetData::PrecomputedPlanetData()
{
    noise2.SetCellularReturnType(FastNoise::Distance);
    noise3.SetCellularReturnType(FastNoise::CellValue);
}

PlanetData::WeatherData PrecomputedPlanetData::get_weather_at_location(const glm::dvec3& location)
{
    // [0, pi / 2] distance to equator
    double d_eq = std::abs(glm::asin(location.z));

    // [0, 1] distance to poles
    double d_pl = 1 - d_eq / glm::half_pi<double>();

    double humidity_noise = noise.GetSimplex(location.x * 121, location.y * 121, location.z * 121) * (d_eq / glm::half_pi<double>());
    double humidity       = cos((d_eq + humidity_noise * 0.5) * 14) * d_pl * d_pl * 0.5 + 0.5;

    // [-1, 1] noise range
    double temperature_noise = (noise.GetSimplex(location.x * 152, location.y * 152, location.z * 434) + noise.GetSimplex(location.x * 42, location.y * 42, location.z * 220)) * 0.5;
    double temperature       = (cos(abs(location.z * glm::half_pi<double>()))) * 80 - 50 + temperature_noise * 10;

    return {
        .humidity = static_cast<float>(humidity),
        .temperature = static_cast<float>(temperature)
    };
}

PlanetData::TectonicData PrecomputedPlanetData::get_tectonic_plate_data_at_location(const glm::dvec3& location)
{
    double c = 1.0 - noise2.GetCellular(location.x * 200, location.y * 200, location.z * 200);

    double c2 = noise2.GetCellular(location.x * 178, location.y * 178, location.z * 178);
    double c3 = noise3.GetCellular(location.x * 178, location.y * 178, location.z * 178);

    return {
        .plate_layer = static_cast<float>((pow(glm::clamp(1 - c2 - 0.2, 0.0, 1.0), 20.0))),
        .mountain_layer = 0
    };
}

PlanetData::RiverData PrecomputedPlanetData::get_river_data_at_location(const glm::dvec3& location)
{
    return {};
}

PlanetData::BiomeDataAtLocation PrecomputedPlanetData::get_biome_data_at_location(const glm::dvec3& location)
{
    return {};
}

float PrecomputedPlanetData::get_height_at_location(const glm::dvec3& location)
{
    return noise.GetSimplex(location.x * 100, location.y * 100, location.z * 100) * 200;
}