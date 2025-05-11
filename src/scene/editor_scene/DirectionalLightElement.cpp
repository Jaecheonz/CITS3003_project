#include "DirectionalLightElement.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/component_wise.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, ElementRef parent) {
    auto default_direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    auto default_color = glm::vec3(1.0f, 1.0f, 1.0f);
    
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "Directional Light",
        default_direction,
        DirectionalLight::create(
            glm::vec3{}, // Will be set in update_instance_data
            glm::vec4{default_color, 1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EntityRenderer::VertexData>("cylinder.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{1.0f}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{default_color, 1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );
    
    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, ElementRef parent, const json& j) {
    // Follow PointLightElement approach for consistency
    auto light_element = new_default(scene_context, parent);
    
    light_element->direction = j["direction"];
    light_element->light->colour = j["colour"];
    light_element->visible = j["visible"];
    light_element->visual_scale = j["visual_scale"];
    
    light_element->update_instance_data();
    return light_element;
}

json EditorScene::DirectionalLightElement::into_json() const {
    return {
        {"name", name},
        {"direction", direction},
        {"colour", light->colour},
        {"visible", visible},
        {"visual_scale", visual_scale},
    };
}

void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    ImGui::Text("Direction");
    bool updated = false;
    glm::vec3 dir = direction;
    if (ImGui::InputFloat3("Direction", &dir[0], "%.2f")) {
        if (glm::length(dir) > 0.001f) {
            direction = glm::normalize(dir);
            updated = true;
        }
    }
    ImGui::DragDisableCursor(scene_context.window);
    ImGui::Spacing();

    ImGui::Text("Light Properties");
    glm::vec4 colour = light->colour;
    updated |= ImGui::ColorEdit3("Colour", &colour[0]);
    ImGui::Spacing();
    updated |= ImGui::DragFloat("Intensity", &colour.a, 0.01f, 0.0f, FLT_MAX);
    light->colour = colour;
    ImGui::DragDisableCursor(scene_context.window);

    ImGui::Spacing();
    ImGui::Text("Visuals");

    updated |= ImGui::Checkbox("Show Visuals", &visible);
    updated |= ImGui::DragFloat("Visual Scale", &visual_scale, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    if (updated) {
        update_instance_data();
    }
}

void EditorScene::DirectionalLightElement::update_instance_data() {
    glm::vec3 up = glm::vec3{0.0f, 1.0f, 0.0f};
    glm::vec3 axis = glm::cross(up, direction);
    float angle = glm::acos(glm::dot(up, direction));

    glm::mat4 rotation = glm::rotate(angle, axis);
    glm::mat4 scale = glm::scale(glm::vec3{visual_scale});

    if (!EditorScene::is_null(parent)) {
        rotation = (*parent)->transform * rotation;
    }

    light->direction = glm::normalize(glm::vec3(rotation * glm::vec4(direction, 0.0f)));

    if (visible) {
        light_arrow->instance_data.model_matrix = rotation * scale;
    } else {
        light_arrow->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
    }

    glm::vec3 norm_col = glm::vec3(light->colour) / glm::compMax(glm::vec3(light->colour));
    light_arrow->instance_data.material.emission_tint = glm::vec4(norm_col, light_arrow->instance_data.material.emission_tint.a);
}

const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}