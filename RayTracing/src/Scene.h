#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace RayTracing {

    struct Material
    {
        glm::vec3 Albedo{1.0f};
        float Roughness = 1.0f;

        glm::vec3 EmissionColor{1.0f};
        float EmissionStrength = 0.0f;
    };

    struct Sphere
    {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        float Radius = 0.5f;

        int MaterialIndex = 0;
    };

    struct Scene
    {
        std::vector<Sphere> Spheres;
        std::vector<Material> Materials;
    };

}