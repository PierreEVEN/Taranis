#include "precomputed_planet_data.hpp"

#include "logger.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

PrecomputedPlanetData::PrecomputedPlanetData()
{
    noise2.SetCellularReturnType(FastNoise::Distance);
    noise3.SetCellularReturnType(FastNoise::CellValue);

    planet_map = PlanetMap<PlanetPixel>(1024);

    for (float x = 0; x <= 1; x += 0.25f)
        for (float y = 0; y <= 1; y += 0.25f)
        {
            glm::vec3 cubf = {x, y, 1};

            auto sph = (glm::vec3)cube_to_sphere(cubf);

            auto cubf2 = (glm::vec3)cubify({sph.x, sph.y, sph.z});

            auto delta = cubf2 - cubf;

            if (abs(delta.x) + abs(delta.y) + abs(delta.z) > 0.0001)
                LOG_DEBUG("{} {}\n\tA = {},{},{}, \n\tB = {},{},{}, \n\tC = {},{},{}\n\tDELTA = {},{},{}\n",
                      x, y,
                      cubf.x, cubf.y, cubf.z,
                      sph.x, sph.y, sph.z,
                      cubf2.x, cubf2.y, cubf2.z,
                      delta.x, delta.y, delta.z);

        }

    for (Face f = static_cast<Face>(0); static_cast<uint32_t>(f) < 6; f = static_cast<Face>(static_cast<uint32_t>(f) + 1))
    {
        for (size_t px = 0; px < 1024llu * 1024llu; ++px)
        {
            auto sphere_pos = cube_to_sphere(planet_map.get_cube_position(f, px));

            double sampled = noise.GetSimplex(sphere_pos.x * 121, sphere_pos.y * 121, sphere_pos.z * 121);

            planet_map[{f, px}].test_val = sampled;
            planet_map[{f, px}].test_rgb = {sphere_pos};
        }
    }




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

    auto cube_loc    = sphere_to_cube_face(location);
    auto sample_data = planet_map.sample(cube_loc);

    double sampled_val = planet_map[{cube_loc.face, sample_data.p1}].test_val * sample_data.v1 +
                         planet_map[{cube_loc.face, sample_data.p2}].test_val * sample_data.v2 +
                         planet_map[{cube_loc.face, sample_data.p3}].test_val * sample_data.v3 +
                         planet_map[{cube_loc.face, sample_data.p4}].test_val * sample_data.v4;

    glm::dvec3 sampled_val_tsts = planet_map[{cube_loc.face, sample_data.p1}].test_rgb * glm::dvec3(sample_data.v1) + planet_map[{cube_loc.face, sample_data.p2}].test_rgb * glm::dvec3(sample_data.v2) +
                         planet_map[{cube_loc.face, sample_data.p3}].test_rgb * glm::dvec3(sample_data.v3) + planet_map[{cube_loc.face, sample_data.p4}].test_rgb * glm::dvec3(sample_data.v4);

    double test_val = noise.GetSimplex(location.x * 121, location.y * 121, location.z * 121);
    return {
        .plate_layer = static_cast<float>(pow(glm::clamp(1 - c2 - 0.2, 0.0, 1.0), 20.0)),
        .mountain_layer = static_cast<float>(sampled_val), 
        .test = glm::vec3(sampled_val_tsts)
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