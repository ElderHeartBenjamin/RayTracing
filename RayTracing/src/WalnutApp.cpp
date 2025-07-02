#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Timer.h"
#include "Walnut/UI/UI.h"

#include "Camera.h"
#include "Renderer.h"

#include <glm/gtc/type_ptr.hpp>

namespace RayTracing {

    class RayTracingLayer : public Walnut::Layer
    {
    public:
        RayTracingLayer()
            : m_Camera(45.0f, 0.1f, 1000.0f) 
        {
            Material& pinkMaterial = m_Scene.Materials.emplace_back();
            pinkMaterial.Albedo = { 1.0f, 0.0f, 1.0f };
            pinkMaterial.Roughness = 0.0f;

            Material& blueMaterial = m_Scene.Materials.emplace_back();
            blueMaterial.Albedo = { 0.2f, 0.3f, 1.0f };
            blueMaterial.Roughness = 0.1f;

            {
                Sphere sphere;
                sphere.Position = { 0.0f, 0.0f, 0.0f };
                sphere.Radius = 1.0f;
                sphere.MaterialIndex = 0;
                m_Scene.Spheres.emplace_back(sphere);
            }

            {
                Sphere sphere;
                sphere.Position = { 0.0f, -101.0f, 0.0f };
                sphere.Radius = 100.0f;
                sphere.MaterialIndex = 1;
                m_Scene.Spheres.emplace_back(sphere);
            }
        }

        virtual void OnUIRender() override
        {
            {
                ImGui::Begin("Settings");

                int treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::TreeNodeEx("Statistics", treeNodeFlags))
                {
                    ImGui::Text("Last render: %.3fms", m_LastRenderTime);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNodeEx("Controls", treeNodeFlags))
                {
                    float cameraSpeed = m_Camera.GetSpeed();
                    if (ImGui::DragFloat("Camera speed", &cameraSpeed, 0.1f, 0.1f, 10.0f)
                        && cameraSpeed >= 0.1f && cameraSpeed <= 10.0f)
                        m_Camera.SetSpeed(cameraSpeed);

                    ImGui::TreePop();
                }

                ImGui::End();
            }

            {
                ImGui::Begin("Scene");

                int treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::TreeNodeEx("Spheres", treeNodeFlags))
                {
                    for (size_t i = 0; i < m_Scene.Spheres.size(); i++)
                    {
                        Sphere& sphere = m_Scene.Spheres[i];

                        ImGui::PushID((int)i);
                        ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
                        ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
                        ImGui::DragInt("Material Index", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

                        ImGui::Separator();
                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNodeEx("Materials", treeNodeFlags))
                {
                    for (size_t i = 0; i < m_Scene.Materials.size(); i++)
                    {
                        Material& material = m_Scene.Materials[i];

                        ImGui::PushID((int)i);
                        ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
                        ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);

                        ImGui::Separator();
                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }

                ImGui::End();
            }

            {
                Walnut::UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Viewport");

                m_ViewportWidth = (uint32_t)ImGui::GetContentRegionAvail().x;
                m_ViewportHeight = (uint32_t)ImGui::GetContentRegionAvail().y;

                auto image = m_Renderer.GetFinalImage();
                if (image)
                    ImGui::Image(image->GetDescriptorSet(), ImVec2((float)image->GetWidth(), (float)image->GetHeight()), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

                ImGui::End();
            }
        }

        virtual void OnUpdate(float ts)
        {
            m_Camera.OnUpdate(ts);
            Render();
        }

        void Render()
        {
            Walnut::Timer timer;

            m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
            m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
            m_Renderer.Render(m_Scene, m_Camera);

            m_LastRenderTime = timer.ElapsedMillis();
        }
    private:
        Camera m_Camera;
        Scene m_Scene;
        Renderer m_Renderer;

        float m_LastRenderTime = 0.0f;
        uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
    };

}

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
    Walnut::ApplicationSpecification spec;
    spec.Width = 2268, spec.Height = 1510;
    spec.Name = "RayTracing";
    spec.CustomTitlebar = true;

    Walnut::Application* app = new Walnut::Application(spec);
    std::shared_ptr<RayTracing::RayTracingLayer> exampleLayer = std::make_shared<RayTracing::RayTracingLayer>();
    app->PushLayer(exampleLayer);
    return app;
}