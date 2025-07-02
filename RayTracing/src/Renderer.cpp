#include "Renderer.h"

namespace RayTracing {

    namespace Utils {

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
    }

    void Renderer::Render(const Scene& scene, const Camera& camera)
    {
        if (!m_FinalImage->GetWidth() || !m_FinalImage->GetHeight())
            return;

        m_ActiveScene = &scene;
        m_ActiveCamera = &camera;

        for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
        {
            for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
            {
                glm::vec4 color = PerPixel(x, y);
                color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
            }
        }

        m_FinalImage->SetData(m_ImageData);
    }

    glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
    {
        Ray ray;
        ray.Origin = m_ActiveCamera->GetPosition();
        ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
        float multiplier = 1.0f;

        int bounces = 5;
        for (int i = 0; i < bounces; i++)
        {
            HitPayload payload = TraceRay(ray);
            if (payload.HitDistance < 0.0f)
            {
                color += skyColor * multiplier;
                break;
            }

            glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
            float lightIntensity = glm::max(glm::dot(payload.WorldNormal, -lightDir), 0.0f);

            const Sphere& closestSphere = m_ActiveScene->Spheres[payload.ObjectIndex];
            const Material& material = m_ActiveScene->Materials[closestSphere.MaterialIndex];
            glm::vec3 sphereColor = material.Albedo;
            sphereColor = sphereColor * lightIntensity;
            color += sphereColor * multiplier;
            multiplier *= 0.5f;

            ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f; // Moving out little bit
            ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
        }

        return glm::vec4(color, 1.0f);
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

            // float t0 = (-b + glm::sqrt(discriminant)) / (a * 2.0f);
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
        glm::vec3 origin = ray.Origin - closestSphere.Position;
        payload.WorldPosition = origin + ray.Direction * hitDistance;
        payload.WorldNormal = glm::normalize(payload.WorldPosition);

        payload.WorldPosition += closestSphere.Position; // Move it back to where it was since we did the subtraction above
        return payload;
    }

    Renderer::HitPayload Renderer::Miss(const Ray& ray)
    {
        HitPayload payload;
        payload.HitDistance = -1.0f;
        return payload;
    }

}