#include "DirectionalLightElement.h"

#include <glm/gtx/normalize_dot.hpp>
#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "New Directional Light",
        glm::vec3{0.0f, -1.0f, 0.0f},
        DirectionalLight::create(
            glm::vec3{0.0f, -1.0f, 0.0f}, // direction
            glm::vec4{1.0f} // color
        )
    );

    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto light_element = new_default(scene_context, parent);

    light_element->direction = j["direction"];
    light_element->light->colour = j["colour"];

    light_element->update_instance_data();
    return light_element;
}

json EditorScene::DirectionalLightElement::into_json() const {
    return {
        {"direction", direction},
        {"colour",    light->colour}
    };
}

void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    bool changed = false;

    ImGui::Text("Direction");
    changed |= ImGui::DragFloat3("Direction Vector", &direction[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);

    ImGui::Spacing();
    ImGui::Text("Light Properties");
    changed |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();
    changed |= ImGui::DragFloat("Intensity", &light->colour.a, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    if (changed) {
        update_instance_data();
    }
}

void EditorScene::DirectionalLightElement::update_instance_data() {
    // Always keep the direction normalized
    light->direction = glm::normalize(direction);

    // No transform matrix needed unless a visual (arrow) is needed
}

const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
