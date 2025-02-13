#include "precomputed_planet_data.hpp"

#include "logger.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

constexpr size_t RES = 1024;

PrecomputedPlanetData::PrecomputedPlanetData()
{
    noise2.SetCellularReturnType(FastNoise::Distance);
    noise3.SetCellularReturnType(FastNoise::CellValue);

    planet_map = PlanetMap<PlanetPixel>(RES);

    for (float x = 0; x <= 1; x += 0.25f)
        for (float y = 0; y <= 1; y += 0.25f)
        {
            glm::vec3 cubf = {x, y, 1};

            auto sph = (glm::vec3)cube_to_sphere(cubf);

            auto cubf2 = (glm::vec3)cubify({sph.x, sph.y, sph.z});

            auto delta = cubf2 - cubf;
        }

    /*
    for (int x = -25; x <= 25; ++x)
    {
        for (int y = -25; y < -24; ++y)
        {
            // [0 - 0.2]
            auto pos_cube = glm::dvec2{x, y} / (5.0 / 2.0) / (double)RES;

            auto sphere_pos = cube_to_sphere({pos_cube, 25});

            auto cube_2 = sphere_to_cube(sphere_pos);

            LOG_DEBUG("{} - {} - {}", pos_cube.x, sphere_pos.x, cube_2.x);



            // [5 - 6]
            glm::dvec2 scaled = (clamp(pos_cube, {-1.0, -1.0}, {1.0, 1.0}) / 2.0 + 0.5) * static_cast<double>(RES - 1);

            float x1 = (scaled.x - std::floor(scaled.x));
            float y2 = (scaled.y - std::floor(scaled.y));

            double v1 = (1 - x1) * (1 - y2),
                   v2 = x1 * (1 - y2),
                   v3 = x1 * y2,
                   v4 = (1 - x1) * y2;

            glm::uvec2 p1 = {
                std::floor(scaled.x),
                std::floor(scaled.y),
            };
            glm::uvec2 p2 = {
                std::ceil(scaled.x),
                std::floor(scaled.y),
            };
            glm::uvec2 p3 = {
                std::ceil(scaled.x),
                std::ceil(scaled.y),
            };
            glm::uvec2 p4 = {
                std::floor(scaled.x),
                std::ceil(scaled.y),
            };

            auto sample_data = planet_map.sample({Face::Front, pos_cube});

            //LOG_WARNING("{} {} => {} {}\n\t{} {} {}\n\t{} {} {}\n\t{} {} {}\n\t{} {} {}", pos_cube.x, pos_cube.y, x1, y2, p1.x, p1.y, v1, p2.x, p2.y, v2, p3.x, p3.y, v3, p4.x, p4.y, v4);

        }
    }*/

    for (Face f = static_cast<Face>(0); static_cast<uint32_t>(f) < 6; f = static_cast<Face>(static_cast<uint32_t>(f) + 1))
    {
        for (size_t px = 0; px < RES * RES; ++px)
        {
            auto cube_pos = planet_map.get_cube_position(f, px);
            auto   sphere_pos = cube_to_sphere(cube_pos);
            double sampled    = noise.GetSimplex(sphere_pos.x * 121, sphere_pos.y * 121, sphere_pos.z * 121);

            planet_map[{f, px}].test_val = sampled;
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

    return {
        .plate_layer = static_cast<float>(pow(glm::clamp(1 - c2 - 0.2, 0.0, 1.0), 20.0)),
        .mountain_layer = static_cast<float>(sampled_val),
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