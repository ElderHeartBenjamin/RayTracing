#pragma once

#include "Walnut/Image.h"
#include "Walnut/Random.h"

#include "Ray.h"
#include "Scene.h"
#include "Camera.h"

#include <glm/glm.hpp>

namespace RayTracing {

    class Renderer
    {
    public:
        struct Settings
        {
            bool Accumulate = true;
        };
    public:
        Renderer() = default;
        ~Renderer();

        void OnResize(uint32_t width, uint32_t height);
        void Render(const Scene& scene, const Camera& camera);
        void ResetFrameIndex() { m_FrameIndex = 1; }

        std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }
        Settings& GetSettings() { return m_Settings; }
    private:
        struct HitPayload
        {
            float HitDistance;
            glm::vec3 WorldPosition;
            glm::vec3 WorldNormal;

            int ObjectIndex;
        };

        glm::vec4 PerPixel(uint32_t x, uint32_t y);
        HitPayload TraceRay(const Ray& ray);
        HitPayload ClosestHit(const Ray& ray, float hitDistance, int objectIndex);
        HitPayload Miss(const Ray& ray);
    private:
        Settings m_Settings;
        std::shared_ptr<Walnut::Image> m_FinalImage;

        uint32_t* m_ImageData = nullptr;
        glm::vec4* m_AccumulationData = nullptr;
        std::vector<uint32_t> m_ImageHorizontalIter;
        std::vector<uint32_t> m_ImageVerticaltalIter;

        uint32_t m_FrameIndex = 1;

        const Scene* m_ActiveScene = nullptr;
        const Camera* m_ActiveCamera = nullptr;
    };

}