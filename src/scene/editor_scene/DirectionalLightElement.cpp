#include "DirectionalLightElement.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/component_wise.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

using namespace EditorScene;

std::unique_ptr<DirectionalLightElement> DirectionalLightElement::new_default(const SceneContext& scene_context, ElementRef parent) {
    auto light = DirectionalLight::create(
        glm::vec3{0.0f, -1.0f, 0.0f},
        glm::vec4{1.0f}
    );

    auto arrow = EmissiveEntityRenderer::Entity::create(
        scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("arrow.obj"),
        EmissiveEntityRenderer::InstanceData{
            glm::mat4{},
            EmissiveEntityRenderer::EmissiveEntityMaterial{glm::vec4{1.0f}}
        },
        EmissiveEntityRenderer::RenderData{
            scene_context.texture_loader.default_white_texture()
        }
    );

    auto element = std::make_unique<DirectionalLightElement>(parent, "New Directional Light", glm::vec3{0.0f, -1.0f, 0.0f}, light, arrow);
    element->update_instance_data();
    return element;
}

std::unique_ptr<DirectionalLightElement> DirectionalLightElement::from_json(const SceneContext& scene_context, ElementRef parent, const json& j) {
    auto element = new_default(scene_context, parent);
    element->direction = glm::normalize(j["direction"]);
    element->light->colour = j["colour"];
    element->visible = j["visible"];
    element->visual_scale = j["visual_scale"];
    element->update_instance_data();
    return element;
}

json DirectionalLightElement::into_json() const {
    return {
        {"direction",    direction},
        {"colour",       light->colour},
        {"visible",      visible},
        {"visual_scale", visual_scale}
    };
}

void DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    ImGui::Text("Light Direction");
    bool updated = ImGui::DragFloat3("Direction", &direction[0], 0.01f);
    if (updated) {
        direction = glm::normalize(direction);
    }

    ImGui::Spacing();
    ImGui::Text("Light Properties");
    updated |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();
    updated |= ImGui::DragFloat("Intensity", &light->colour.a, 0.01f, 0.0f, FLT_MAX);

    ImGui::Spacing();
    ImGui::Text("Visuals");
    updated |= ImGui::Checkbox("Show Visuals", &visible);
    updated |= ImGui::DragFloat("Visual Scale", &visual_scale, 0.01f, 0.0f, FLT_MAX);

    if (updated) {
        update_instance_data();
    }
}

void DirectionalLightElement::update_instance_data() {
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

const char* DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
