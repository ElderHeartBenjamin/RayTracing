#include "Renderer.h"

#include <execution>

namespace RayTracing {

#define RT_ENABLE_MT 1

    namespace Utils {

        static float FastRandom(uint32_t& state)
        {
            state = state * 747796405 + 2891336453;
            uint32_t result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
            result = (result >> 22) ^ result;
            return (float)result / 4294967295.0f;
        }

        static float RandomValueNormalDistribution(uint32_t& state)
        {
            float theta = 2 * 3.1415926f * FastRandom(state);
            float rho = glm::sqrt(-2 * glm::log(FastRandom(state)));
            return rho * glm::cos(theta);
        }

        static glm::vec3 RandomDirection(uint32_t& state)
        {
            float x = RandomValueNormalDistribution(state);
            float y = RandomValueNormalDistribution(state);
            float z = RandomValueNormalDistribution(state);
            return glm::normalize(glm::vec3(x, y, z));
        }

        static uint32_t ConvertToRGBA(const glm::vec4& color)
        {
            uint8_t r = (uint8_t)(color.r * 255.0f);
            uint8_t g = (uint8_t)(color.g * 255.0f);
            uint8_t b = (uint8_t)(color.b * 255.0f);
            uint8_t a = (uint8_t)(color.a * 255.0f);

            uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
            return result;
        }

    }
    
    Renderer::~Renderer()
    {
        delete[] m_ImageData;
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (m_FinalImage)
        {
            if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
                return;

            m_FinalImage->Resize(width, height);
        }
        else
        {
            m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
        }

        delete[] m_ImageData;
        m_ImageData = new uint32_t[width * height];

        delete[] m_AccumulationData;
        m_AccumulationData = new glm::vec4[width * height];

        m_ImageHorizontalIter.resize(width);
        m_ImageVerticaltalIter.resize(height);
        for (uint32_t i = 0; i < width; i++)
            m_ImageHorizontalIter[i] = i;
        for (uint32_t i = 0; i < height; i++)
            m_ImageVerticaltalIter[i] = i;
    }

    void Renderer::Render(const Scene& scene, const Camera& camera)
    {
        if (!m_FinalImage->GetWidth() || !m_FinalImage->GetHeight())
            return;

        if (m_FrameIndex == 1)
            memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

        m_ActiveScene = &scene;
        m_ActiveCamera = &camera;

#if RT_ENABLE_MT
        std::for_each(std::execution::par, m_ImageVerticaltalIter.begin(), m_ImageVerticaltalIter.end(), [this](uint32_t y)
        {
            std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(), [this, y](uint32_t x)
            {
                glm::vec4 color = PerPixel(x, y);
                m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

                glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()] / (float)m_FrameIndex;
                accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulateColor);
            });
        });
#else
        for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
        {
            for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
            {
                glm::vec4 color = PerPixel(x, y);
                color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
            }
        }
#endif

        if (m_Settings.Accumulate)
            m_FrameIndex++;
        else
            m_FrameIndex = 1;

        m_FinalImage->SetData(m_ImageData);
    }

    glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
    {
        Ray ray;
        ray.Origin = m_ActiveCamera->GetPosition();
        ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 incomingLight = glm::vec3(0.0f);

        uint32_t seed = x + y * m_FinalImage->GetWidth();
        seed *= m_FrameIndex;

        int bounces = 5;
        for (int i = 0; i < bounces; i++)
        {
            seed += i;

            HitPayload payload = TraceRay(ray);
            if (payload.HitDistance >= 0.0f)
            {
                const Sphere& closestSphere = m_ActiveScene->Spheres[payload.ObjectIndex];
                const Material& material = m_ActiveScene->Materials[closestSphere.MaterialIndex];

                ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f; // Moving out little bit
                glm::vec3 diffuseDir = glm::normalize(payload.WorldNormal + Utils::RandomDirection(seed));
                glm::vec3 reflectDir = glm::reflect(ray.Direction, payload.WorldNormal);
                ray.Direction = glm::normalize(glm::mix(reflectDir, diffuseDir, material.Roughness));

                incomingLight += (material.EmissionColor * material.EmissionStrength) * color;
                color *= material.Albedo;
            }
            else
            {
                glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
                incomingLight += skyColor * color;
                break;
            }
        }

        return glm::vec4(incomingLight, 1.0f);
    }

    Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
    {
        // Circle intercept calculation
        // (bx ^ 2 + by ^ 2)t ^ 2 + (2axbx + 2ayby)t + (ax ^ 2 + ay ^ 2 - r ^ 2) = 0
        // a = ray origin (position of the camera)
        // b = ray direction
        // r = radius

        if (m_ActiveScene->Spheres.empty())
            return Miss(ray);

        int closestSphere = -1;
        float hitDistance = std::numeric_limits<float>::max();

        for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
        {
            const Sphere& sphere = m_ActiveScene->Spheres[i];
            glm::vec3 origin = ray.Origin - sphere.Position;

            float a = glm::dot(ray.Direction, ray.Direction); // (bx ^ 2 + by ^ 2 + bz ^ 2)
            float b = 2.0f * glm::dot(origin, ray.Direction); // (2axbx + 2ayby + 2azbz)
            float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius; // (ax ^ 2 + ay ^ 2 + az ^ 2 - r ^ 2)

            float discriminant = b * b - 4.0f * a * c;
            if (discriminant < 0) 
                continue;

            float t0 = (-b + glm::sqrt(discriminant)) / (a * 2.0f);
            float t1 = (-b - glm::sqrt(discriminant)) / (a * 2.0f);

            if (t1 > 0.0f && t1 < hitDistance) 
            {
                hitDistance = t1;
                closestSphere = (int)i;
            }
        }

        if (closestSphere < 0)
            return Miss(ray);

        return ClosestHit(ray, hitDistance, closestSphere);
    }

    Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
    {
        HitPayload payload;
        payload.HitDistance = hitDistance;
        payload.ObjectIndex = objectIndex;

        const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];
        payload.WorldPosition = ray.Origin + ray.Direction * hitDistance;
        payload.WorldNormal = glm::normalize(payload.WorldPosition - closestSphere.Position);
        return payload;
    }

    Renderer::HitPayload Renderer::Miss(const Ray& ray)
    {
        HitPayload payload;
        payload.HitDistance = -1.0f;
        return payload;
    }

}