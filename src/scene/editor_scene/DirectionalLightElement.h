#ifndef DIRECTIONAL_LIGHT_ELEMENT_H
#define DIRECTIONAL_LIGHT_ELEMENT_H

#include "SceneElement.h"
#include "scene/SceneContext.h"

namespace EditorScene {
    class DirectionalLightElement : public SceneElement {
    private:
        glm::vec3 position{0.0f, 1.0f, 0.0f};

    public:
        static constexpr const char* ELEMENT_TYPE_NAME = "Directional Light";

        glm::vec3 direction{0.0f, -1.0f, 0.0f};
        bool visible = true;
        float visual_scale = 0.1f;

        std::shared_ptr<DirectionalLight> light;
        std::shared_ptr<EmissiveEntityRenderer::Entity> light_arrow;

        DirectionalLightElement(const ElementRef& parent, std::string name, glm::vec3 dir, std::shared_ptr<DirectionalLight> light, std::shared_ptr<EmissiveEntityRenderer::Entity> arrow)
            : SceneElement(parent, std::move(name)), direction(glm::normalize(dir)), light(std::move(light)), light_arrow(std::move(arrow)) {}

        static std::unique_ptr<DirectionalLightElement> new_default(const SceneContext& scene_context, ElementRef parent);
        static std::unique_ptr<DirectionalLightElement> from_json(const SceneContext& scene_context, ElementRef parent, const json& j);

        [[nodiscard]] json into_json() const override;

        void add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) override;
        void update_instance_data() override;

        void add_to_render_scene(MasterRenderScene& target_render_scene) override {
            target_render_scene.insert_entity(light_arrow);
            target_render_scene.insert_light(light);
        }

        void remove_from_render_scene(MasterRenderScene& target_render_scene) override {
            target_render_scene.remove_entity(light_arrow);
            target_render_scene.remove_light(light);
        }

        [[nodiscard]] const char* element_type_name() const override;
    };
}

#endif // DIRECTIONAL_LIGHT_ELEMENT_H
