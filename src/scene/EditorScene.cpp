#include "EditorScene.h"

#include <tinyfiledialogs/tinyfiledialogs.h>

#include "rendering/imgui/ImGuiManager.h"
#include "rendering/cameras/PanningCamera.h"
#include "rendering/cameras/FlyingCamera.h"

#include "editor_scene/EntityElement.h"
#include "editor_scene/AnimatedEntityElement.h"
#include "editor_scene/EmissiveEntityElement.h"
#include "editor_scene/PointLightElement.h"
#include "editor_scene/DirectionalLightElement.h"
#include "editor_scene/GroupElement.h"
#include "scene/SceneContext.h"

EditorScene::EditorScene::EditorScene() {
    /// Initialise the scene_root and specify nothing selected
    scene_root = std::make_shared<std::list<std::unique_ptr<SceneElement>>>(std::list<std::unique_ptr<SceneElement>>{});
    selected_element = NullElementRef;
}

void EditorScene::EditorScene::open(const SceneContext& scene_context) {
    /// Setup the camera with the default state
    camera = std::make_unique<PanningCamera>(init_distance, init_focus_point, init_pitch, init_yaw, init_near, init_fov);
    /// Tell the scene to use this camera to camera the view and projection matrices.
    /// Just calling this once to make sure the initial values are correct.
    render_scene.use_camera(*camera);

    /// Create a EditorScene::EntityElement to control the Entity of the default ground plane
    auto plane = std::make_unique<EntityElement>(
        NullElementRef,
        "Ground Plane",
        glm::vec3{0.0f, -0.01f, 0.0f}, // Place it slightly below zero to prevent z fighting with anything placed at y=0
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{10.0f, 1.0f, 10.0f},
        EntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EntityRenderer::VertexData>("double_plane.obj"),
            EntityRenderer::InstanceData{
                glm::mat4{}, // Set via update_instance_data()
                EntityRenderer::EntityMaterial{
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    128.0f,
                }
            },
            EntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture(),
                scene_context.texture_loader.default_white_texture()
            }
        )
    );

    /// Update the transform, to propagate the position, rotation, scale, etc.. from the SceneElement to the actual Entity
    plane->update_instance_data();
    /// Add the SceneElement to the render scene, and add to the root of the tree
    plane->add_to_render_scene(render_scene);
    scene_root->push_back(std::move(plane));

    auto default_light_pos = glm::vec3(1.0f, 2.0f, 1.0f);
    auto default_light_col = glm::vec3(1.0f);

    /// Crate the default point light, which also controls the light sphere
    auto default_light = std::make_unique<PointLightElement>(
        NullElementRef,
        "Default Point Light",
        default_light_pos,
        PointLight::create(
            glm::vec3{}, // Set via update_instance_data()
            glm::vec4{default_light_col, 1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EntityRenderer::VertexData>("sphere.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{1.0f}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{default_light_col, 1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );

    /// Update the transform, to propagate the position, rotation, scale, etc.. from the SceneElement to the actual Entity
    default_light->update_instance_data();
    /// Add the SceneElement to the render scene, and add to the root of the tree
    default_light->add_to_render_scene(render_scene);
    scene_root->push_back(std::move(default_light));

    /// Setup all the generates

    /// All the entity generators, new entity types must be registered here to be able to be created in the UI
    entity_generators = {
        {EntityElement::ELEMENT_TYPE_NAME,         [](const SceneContext& scene_context, ElementRef parent) { return EntityElement::new_default(scene_context, parent); }},
        {AnimatedEntityElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent) { return AnimatedEntityElement::new_default(scene_context, parent); }},
        {EmissiveEntityElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent) { return EmissiveEntityElement::new_default(scene_context, parent); }},
    };

    /// All the light generators, new light types must be registered here to be able to be created in the UI
    light_generators = {
        {PointLightElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent) { return PointLightElement::new_default(scene_context, parent); }},
        // task h
        {DirectionalLightElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent) { return DirectionalLightElement::new_default(scene_context, parent); }},
    };

    /// All the element generators, new element types must be registered here to be able to be loaded from json
    json_generators = {
        {EntityElement::ELEMENT_TYPE_NAME,         [](const SceneContext& scene_context, ElementRef parent, const json& j) { return EntityElement::from_json(scene_context, parent, j); }},
        {AnimatedEntityElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent, const json& j) { return AnimatedEntityElement::from_json(scene_context, parent, j); }},
        {EmissiveEntityElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent, const json& j) { return EmissiveEntityElement::from_json(scene_context, parent, j); }},
        {PointLightElement::ELEMENT_TYPE_NAME,     [](const SceneContext& scene_context, ElementRef parent, const json& j) { return PointLightElement::from_json(scene_context, parent, j); }},
        // task h
        {DirectionalLightElement::ELEMENT_TYPE_NAME, [](const SceneContext& scene_context, ElementRef parent, const json& j) { return DirectionalLightElement::from_json(scene_context, parent, j); }},
        {GroupElement::ELEMENT_TYPE_NAME,          [](const SceneContext&, ElementRef parent, const json& j) { return GroupElement::from_json(parent, j); }},
    };
}

std::pair<TickResponseType, std::shared_ptr<SceneInterface>> EditorScene::EditorScene::tick(float delta_time, const SceneContext& scene_context) {
    /// If the `Esc` key was pressed this tick, then tell the scene manager to exit
    if (scene_context.window.was_key_pressed(GLFW_KEY_ESCAPE)) {
        return {TickResponseType::Exit, nullptr};
    }

    /// If the 'V' key was pressed this tick, then cycle the camera mode
    if (scene_context.window.was_key_pressed(GLFW_KEY_V)) {
        switch (camera_mode) {
            case CameraMode::Panning:
                set_camera_mode(CameraMode::Flying);
                break;
            case CameraMode::Flying:
                set_camera_mode(CameraMode::Panning);
                break;
        }
    }

    /// If ImGUI should be enabled, then add the two windows
    if (scene_context.imgui_enabled) {
        add_imgui_selection_editor(scene_context);
        add_imgui_scene_hierarchy(scene_context);
        add_imgui_brush_tool_section(scene_context); // Add brush tool UI
    }

    /// Default to telling the SceneManager to continue ticking
    return {TickResponseType::Continue, nullptr};
}

void EditorScene::EditorScene::add_imgui_options_section() {
    /// Add a section to the ImGUI menu
    if (ImGui::CollapsingHeader("Scene Settings")) {
        // Add radio buttons to switch between the camera modes
        ImGui::Text("Camera Selection (v)");
        if (ImGui::RadioButton("Panning Camera", camera_mode == CameraMode::Panning)) {
            set_camera_mode(CameraMode::Panning);
        }
        if (ImGui::RadioButton("Flying Camera", camera_mode == CameraMode::Flying)) {
            set_camera_mode(CameraMode::Flying);
        }
        ImGui::Separator();
    }
}

MasterRenderScene& EditorScene::EditorScene::get_render_scene() {
    /// Only 1 RenderScene so always just return that
    return render_scene;
}

CameraInterface& EditorScene::EditorScene::get_camera() {
    /// Return the current camera
    return *camera;
}

void EditorScene::EditorScene::close(const SceneContext& /*scene_context*/) {
    // Free up memory by dropping handles
    render_scene = {};
    scene_root->clear();
}

void EditorScene::EditorScene::set_camera_mode(CameraMode new_camera_mode) {
    /// Extract the camera orientation and use that to switch cameras
    auto orientation = camera->save_properties();
    switch (new_camera_mode) {
        case CameraMode::Panning:
            camera = std::make_unique<PanningCamera>(init_distance, init_focus_point, init_pitch, init_yaw, init_near, init_fov);
            break;
        case CameraMode::Flying:
            camera = std::make_unique<FlyingCamera>(init_position, init_pitch, init_yaw, init_near, init_fov);
            break;
    }
    camera->load_properties(orientation);
    this->camera_mode = new_camera_mode;
}

void EditorScene::EditorScene::add_imgui_selection_editor(const SceneContext& scene_context) {
    /// Create an ImGUI window for editing the properties of the currently selected SceneElement.
    if (ImGui::Begin("Selection Editor", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        if (is_null(selected_element)) {
            ImGui::Text("No Element Selected");
        } else {
            /// Adds the SceneElements custom property editors
            (*selected_element)->add_imgui_edit_section(render_scene, scene_context);

            /// If it's animated, add those property editors
            auto* animated_selected_element = dynamic_cast<AnimatedEntityElement*>(selected_element->get());
            if (animated_selected_element != nullptr) {
                animated_selected_element->add_animation_imgui_edit_section(render_scene, scene_context);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            /// Add a general check box to enable/disable the SceneElement, when disabled it is invisible, and
            /// lights provide no light.
            ImGui::Text("Control");
            bool enabled = (*selected_element)->enabled;
            if (ImGui::Checkbox("Enabled", &enabled)) {
                visit_children_and_root(selected_element, [enabled, this](SceneElement& element) {
                    if (enabled && !element.enabled) {
                        element.enabled = true;
                        element.add_to_render_scene(render_scene);
                    } else if (!enabled && element.enabled) {
                        element.enabled = false;
                        element.remove_from_render_scene(render_scene);
                    }
                });
            }
        }
    }
    ImGui::End();
}

void EditorScene::EditorScene::add_imgui_scene_hierarchy(const SceneContext& scene_context) {
    /// Add a control representing the scene tree/hierarchy
    if (ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        /// Calculate where to put or remove items
        auto parent = selected_element;
        auto list = scene_root;
        if (!is_null(selected_element)) {
            list = (*selected_element)->get_children();
        }
        auto insert_at = parent;
        if (list == nullptr) {
            parent = (*parent)->parent;
            if (is_null(parent)) {
                list = scene_root;
            } else {
                list = (*parent)->get_children();
            }
            if (!is_null(insert_at) && insert_at != list->end()) {
                ++insert_at;
            } else {
                insert_at = list->end();
            }
        } else {
            insert_at = list->end();
        }

        /// Add a combo box to create a new element, auto fills from the list of entity_generators
        ImGui::PushItemWidth(110.0f);
        if (ImGui::BeginCombo("##New Entity Combo", "New Entity")) {
            for (const auto& gen: entity_generators) {
                if (ImGui::Selectable(gen.first.c_str())) {
                    try {
                        auto new_entity = gen.second(scene_context, parent);
                        new_entity->add_to_render_scene(render_scene);
                        selected_element = list->insert(insert_at, std::move(new_entity));
                    } catch (const std::exception& e) {
                        std::cerr << "Error while trying to add new Entity:" << std::endl;
                        std::cerr << e.what() << std::endl;
                    }
                }
            }

            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        /// Add a combo box to create a new light, auto fills from the list of light_generators
        ImGui::PushItemWidth(110.0f);
        if (ImGui::BeginCombo("##New Light Combo", "New Light")) {
            for (const auto& gen: light_generators) {
                if (ImGui::Selectable(gen.first.c_str())) {
                    try {
                        auto new_light = gen.second(scene_context, parent);
                        new_light->add_to_render_scene(render_scene);
                        selected_element = list->insert(insert_at, std::move(new_light));
                    } catch (const std::exception& e) {
                        std::cerr << "Error while trying to add new Light:" << std::endl;
                        std::cerr << e.what() << std::endl;
                    }
                }
            }

            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        /// Add a button to create a new special group SceneElement, which allows you to group multiple SceneElements
        if (ImGui::Button("New Group")) {
            auto new_group = std::make_unique<GroupElement>(
                parent,
                "New Group"
            );

            new_group->update_instance_data();
            selected_element = list->insert(insert_at, std::move(new_group));
        }

        ImGui::SameLine();

        bool has_multi_selection = !multi_selected_elements.empty();
        if (!has_multi_selection) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
        }

        if (ImGui::Button("Delete Selected") && has_multi_selection) {
            for (auto& ref : multi_selected_elements) {
                if (is_null(ref)) continue;

                visit_children(ref, [&](SceneElement& element) {
                    if (element.enabled) element.remove_from_render_scene(render_scene);
                });

                (*ref)->remove_from_render_scene(render_scene);
                auto p = (*ref)->parent;
                if (!is_null(p)) {
                    (*p)->get_children()->erase(ref);
                } else {
                    scene_root->erase(ref);
                }
            }

            selected_element = NullElementRef;
            multi_selected_elements.clear();
        }
        ImGui::PopStyleColor(3);

        ImGui::Separator();

        /// Generate The Tree
        {
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed;

            auto new_selected = NullElementRef;

            std::function<void(ElementList&)> process_children;

            process_children = [&](ElementList& children) {
                for (auto iter = children->begin(); iter != children->end(); iter++) {
                    const auto& element = *iter;

                    ImGuiTreeNodeFlags node_flags = base_flags;

                    // Check multi-selection
                    bool is_multi_selected = multi_selected_elements.count(iter) > 0;
                    if (is_multi_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

                    auto grand_children = element->get_children();
                    if (grand_children == nullptr)
                        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                    // Optional style change for selected
                    if (is_multi_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_HeaderHovered));

                    std::string name = Formatter() << element->name.c_str();
                    if (!element->enabled) name += " [Disabled]";
                    name += Formatter() << "##" << (void*)element.get();

                    bool node_open = ImGui::TreeNodeEx(name.c_str(), node_flags | ImGuiTreeNodeFlags_DefaultOpen);

                    // Handle click selection
                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        if (ImGui::GetIO().KeyCtrl) {
                            // Toggle in multi-selection set
                            if (iter != children->end()) {
                                if (multi_selected_elements.count(iter)) {
                                    multi_selected_elements.erase(iter);
                                } else {
                                    multi_selected_elements.insert(iter);
                                }
                            }
                        } else {
                            // Single selection
                            multi_selected_elements.clear();
                            if (iter != children->end()) {
                                multi_selected_elements.insert(iter);
                                selected_element = iter;
                            }
                        }
                    }

                    if (is_multi_selected)
                        ImGui::PopStyleColor();

                    if (node_open && grand_children != nullptr) {
                        process_children(grand_children);
                        ImGui::TreePop();
                    }
                }
            };

            process_children(scene_root);

            // Still allow direct update for single selected (used elsewhere)
            if (!multi_selected_elements.empty()) {
                selected_element = *multi_selected_elements.begin();
            } else {
                selected_element = NullElementRef;
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();

        /// Add the buttons for importing and exporting the scene from/to json files.
        ImGui::Text("Import & Export Editor Scene File");
        ImGui::Spacing();

        bool ctrl_is_pressed = scene_context.window.is_key_pressed(GLFW_KEY_LEFT_CONTROL) || scene_context.window.is_key_pressed(GLFW_KEY_RIGHT_CONTROL);
        bool shift_is_pressed = scene_context.window.is_key_pressed(GLFW_KEY_LEFT_SHIFT) || scene_context.window.is_key_pressed(GLFW_KEY_RIGHT_SHIFT);

        if (ImGui::Button("Open (Ctrl + O)") || (scene_context.window.was_key_pressed(GLFW_KEY_O) && ctrl_is_pressed && !shift_is_pressed)) {
            load_from_json_file(scene_context);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save (Ctrl + S)") || (scene_context.window.was_key_pressed(GLFW_KEY_S) && ctrl_is_pressed && !shift_is_pressed)) {
            save_to_json_file();
        }

        ImGui::SameLine();

        if (ImGui::Button("Save As (Ctrl + Shift + S)") || (scene_context.window.was_key_pressed(GLFW_KEY_S) && ctrl_is_pressed && shift_is_pressed)) {
            const char* filter = "*.json";
            const auto init_path = (std::filesystem::current_path() / "scene.json").string();
            const char* path = tinyfd_saveFileDialog("Save Scene", init_path.c_str(), 1, &filter, "Json Files");
            if (path != nullptr) {
                save_path = path;
                save_to_json_file();
            }
        }

        if (save_path.has_value()) {
            ImGui::InputText("File Path", &save_path.value(), ImGuiInputTextFlags_ReadOnly);
            scene_context.window.set_title_suffix(Formatter() << "Open File: [" << save_path.value() << "]");
        } else {
            static std::string none{};
            ImGui::InputText("File Path", &none, ImGuiInputTextFlags_ReadOnly);
            scene_context.window.set_title_suffix(std::nullopt);
        }
    }
    ImGui::End();
}

void EditorScene::EditorScene::visit_children(ElementRef root, const std::function<void(SceneElement&)>& visit) {
    if (is_null(root)) {
        return;
    }

    auto children = (*root)->get_children();
    if (children != nullptr) {
        for (auto iter = children->begin(); iter != children->end(); ++iter) {
            visit_children_and_root(iter, visit);
        }
    }
}

void EditorScene::EditorScene::visit_children_and_root(ElementRef root, const std::function<void(SceneElement&)>& visit) {
    if (is_null(root)) {
        return;
    }

    visit(**root);

    auto children = (*root)->get_children();
    if (children != nullptr) {
        for (auto iter = children->begin(); iter != children->end(); ++iter) {
            visit_children_and_root(iter, visit);
        }
    }
}

json EditorScene::EditorScene::element_to_labelled_json(const SceneElement& element) {
    json j = element.into_json();
    j["label"] = element.element_type_name();
    element.store_json(j);

    if (j.contains("error")) {
        std::cerr << "Unable to save element in a loadable manor due to error. Error:" << std::endl;
        std::cerr << j["error"] << std::endl;
    }

    auto children = element.get_children();
    if (children != nullptr) {
        json children_json = json::array();

        for (auto& child: *children) {
            children_json.push_back(element_to_labelled_json(*child));
        }

        j["children"] = children_json;
    }

    return j;
}

void EditorScene::EditorScene::add_labelled_json_element(const SceneContext& scene_context, ElementRef parent, const ElementList& list, const json& j) {
    if (j.contains("error")) {
        std::cerr << "Unable to load element due to error, so skipping. Error:" << std::endl;
        std::cerr << j["error"] << std::endl;
        return;
    }

    std::string label = j["label"];

    auto gen = json_generators.find(label);
    if (gen == json_generators.end()) {
        std::cerr << "No generator for label: [" << label << "]" << std::endl;
        return;
    }

    ElementRef ref;
    {
        auto element = gen->second(scene_context, parent, j);
        element->load_json(j);

        element->add_to_render_scene(render_scene);
        list->push_back(std::move(element));
        ref = list->end();
        ref--;
    }

    if (j.contains("children")) {
        for (const auto& child: j["children"]) {
            add_labelled_json_element(scene_context, ref, (*ref)->get_children(), child);
        }
    }
}

void EditorScene::EditorScene::save_to_json_file() {
    auto old_path = save_path;

    if (!save_path.has_value()) {
        const char* filter = "*.json";
        const auto init_path = (std::filesystem::current_path() / "scene.json").string();
        const char* path = tinyfd_saveFileDialog("Save Scene", init_path.c_str(), 1, &filter, "Json Files");
        if (path == nullptr) return;
        save_path = path;
    }

    std::optional<std::filesystem::path> temp = std::nullopt;
    if (std::filesystem::exists(save_path.value())) {

        temp = std::tmpnam(nullptr);
        std::filesystem::rename(save_path.value(), temp.value());
    }

    try {
        json j = json::array();

        for (auto& iter: *scene_root) {
            j.push_back(element_to_labelled_json(*iter));
        }

        std::filesystem::create_directories(std::filesystem::path(save_path.value()).parent_path());
        std::ofstream file(save_path.value());
        file << j.dump(4);
        file.flush();
    } catch (const std::exception& e) {
        if (std::filesystem::exists(save_path.value())) {
            std::filesystem::remove(save_path.value());
        }
        if (temp.has_value()) {
            std::filesystem::rename(temp.value(), save_path.value());
        }
        std::swap(save_path, old_path);

        std::cerr << "Failed to save to file: [" << old_path.value() << "]" << std::endl;
        std::cerr << "Error:" << std::endl;
        std::cerr << e.what() << std::endl;

        tinyfd_messageBox("Failed to save to File", "See Console For Error", "ok", "error", 1);
    }

    if (temp.has_value()) {
        std::filesystem::remove(temp.value());
    }
}

void EditorScene::EditorScene::load_from_json_file(const SceneContext& scene_context) {
    const auto init_path = (std::filesystem::current_path() / "scene.json").string();

#ifdef __APPLE__
    // Apparently the file filter doesn't work properly on Mac?
    // Feel free to re-enable if you want to try it, but I have disabled it on Mac for now.

    const char* path = tinyfd_openFileDialog("Open Scene", init_path.c_str(), 0, nullptr, nullptr, false);
#else
    const char* filter = "*.json";
    const char* path = tinyfd_openFileDialog("Open Scene", init_path.c_str(), 1, &filter, "Json Files", false);
#endif

    if (path == nullptr) return;
    auto old_path = save_path;
    save_path = path;

    MasterRenderScene old_render_scene{};
    ElementList old_scene_root = std::make_shared<std::list<std::unique_ptr<SceneElement>>>(std::list<std::unique_ptr<SceneElement>>{});;
    auto old_selected_element = selected_element;
    std::swap(render_scene, old_render_scene);
    std::swap(scene_root, old_scene_root);
    try {
        selected_element = NullElementRef;

        std::ifstream f(save_path.value());
        json data = json::parse(f);

        for (const auto& item: data) {
            add_labelled_json_element(scene_context, NullElementRef, scene_root, item);
        }

        for (auto& item: *scene_root) {
            item->update_instance_data();
        }
    } catch (const std::exception& e) {
        std::swap(save_path, old_path);
        render_scene = std::move(old_render_scene);
        scene_root = old_scene_root;
        selected_element = old_selected_element;

        std::cerr << "Failed to open file: [" << old_path.value() << "]" << std::endl;
        std::cerr << "Error:" << std::endl;
        std::cerr << e.what() << std::endl;

        tinyfd_messageBox("Failed to open File", "See Console For Error", "ok", "error", 1);
    }
}

void EditorScene::EditorScene::add_imgui_brush_tool_section(const SceneContext& scene_context) {
    if (ImGui::CollapsingHeader("Brush Tool")) {
        static bool brush_enabled = false;
        static float brush_size = 1.0f;
        static int brush_mode = 0;
        static int spawn_density = 1;
        static float y_offset = 0.0f;
        static std::string selected_entity = "Entity";
        static std::unique_ptr<SceneElement> template_entity = nullptr;
        const char* brush_modes[] = { "Once per Click", "Continuous Hold" };


        ImGui::Checkbox("Enable Brush Tool", &brush_enabled);
        ImGui::SliderFloat("Brush Size", &brush_size, 0.1f, 10.0f);
        ImGui::Combo("Brush Mode", &brush_mode, brush_modes, IM_ARRAYSIZE(brush_modes));
        ImGui::SliderInt("Spawn Density", &spawn_density, 1, 20);
        ImGui::SliderFloat("Y Offset", &y_offset, -10.0f, 10.0f, "%.2f");

        // Entity type selection
        if (ImGui::BeginCombo("Entity Type", selected_entity.c_str())) {
            for (const auto& gen : entity_generators) {
                if (ImGui::Selectable(gen.first.c_str(), selected_entity == gen.first)) {
                    selected_entity = gen.first;
                    // Update template entity when type changes
                    const std::string& sel = selected_entity;
                    auto found = std::find_if(entity_generators.begin(), entity_generators.end(),
                        [&sel](const auto& pair) { return pair.first == sel; });
                    if (found != entity_generators.end()) {
                        template_entity = found->second(scene_context, NullElementRef);
                    }
                }
            }
            ImGui::EndCombo();
        }

        // If no template entity yet, create one for the default type
        if (!template_entity) {
            const std::string& sel = selected_entity;
            auto found = std::find_if(entity_generators.begin(), entity_generators.end(),
                [&sel](const auto& pair) { return pair.first == sel; });
            if (found != entity_generators.end()) {
                template_entity = found->second(scene_context, NullElementRef);
            }
        }

        // Show property editor for the template entity
        if (template_entity) {
            ImGui::Separator();
            ImGui::Text("Template Properties");
            template_entity->add_imgui_edit_section(render_scene, scene_context);
        }

        if (brush_enabled) {
            handle_brush_tool(scene_context, brush_size, spawn_density, selected_entity.c_str(), template_entity.get(), brush_mode, y_offset);

        }
    }
}

// Update handle_brush_tool to accept a template entity and copy its properties
void EditorScene::EditorScene::handle_brush_tool(const SceneContext& scene_context, float brush_size, int spawn_density, const char* entity_type, SceneElement* template_entity, int brush_mode, float y_offset) {
    // static state lives across frames:
    static bool was_mouse_down = false;
    static float last_spawn_time = 0.0f;
    const bool mouse_down = ImGui::IsMouseDown(0);
    const float now = ImGui::GetTime(); // seconds since app start

    // compute spawn “permission”:
    bool do_spawn = false;

    if (brush_mode == 0) {
        // Once per click
        if (mouse_down && !was_mouse_down) {
            do_spawn = true;
        }
    } else if (brush_mode == 1) {
        // Continuous spawn
        const float spawn_interval = 0.f; // seconds
        if (mouse_down && (now - last_spawn_time) >= spawn_interval) {
            do_spawn = true;
            last_spawn_time = now;
        }
    }

    was_mouse_down = mouse_down;

    if (!do_spawn)
        return;
    
    // OK, we’re going to spawn `spawn_density` copies at this spot:
    auto mouse_pos = ImGui::GetMousePos();
    glm::vec3 world_position = calculate_world_position(mouse_pos, scene_context, y_offset);    
    for (int i = 0; i < spawn_density; ++i) {
        glm::vec3 spawn_position = world_position;
        spawn_position.y = y_offset;

        // find the right generator
        auto gen_it = std::find_if(entity_generators.begin(), entity_generators.end(),
            [entity_type](const auto& pair) {
                return pair.first == entity_type;
            });
        if (gen_it == entity_generators.end()) continue;

        auto new_entity = gen_it->second(scene_context, NullElementRef);

        // copy template props
        if (template_entity) {
            json props = template_entity->into_json();
            if (auto* e = dynamic_cast<EntityElement*>(new_entity.get())) {
                e->load_json(props, scene_context);
            } else if (auto* a = dynamic_cast<AnimatedEntityElement*>(new_entity.get())) {
                a->load_json(props);
            } else if (auto* em = dynamic_cast<EmissiveEntityElement*>(new_entity.get())) {
                em->load_json(props);
            }
        }

        // place it
        if (auto* e = dynamic_cast<EntityElement*>(new_entity.get())) {
            e->set_position(spawn_position);
        }

        new_entity->update_instance_data();
        new_entity->add_to_render_scene(render_scene);
        scene_root->push_back(std::move(new_entity));
    }
}

glm::vec3 EditorScene::EditorScene::calculate_world_position(const ImVec2& mouse_pos, const SceneContext& scene_context, float y_offset) {
    // Convert mouse position to normalized device coordinates
    float x = (2.0f * mouse_pos.x) / scene_context.window.get_window_width() - 1.0f;
    float y = 1.0f - (2.0f * mouse_pos.y) / scene_context.window.get_window_height();
    glm::vec4 ray_clip = glm::vec4(x, y, -1.0f, 1.0f);

    // Transform to eye space
    glm::vec4 ray_eye = glm::inverse(camera->get_projection_matrix()) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

    // Transform to world space
    glm::vec3 ray_world = glm::normalize(glm::vec3(glm::inverse(camera->get_view_matrix()) * ray_eye));
    glm::vec3 cam_pos = camera->get_position();

    // Ray-plane intersection: plane normal (0,1,0), point (any x, y_offset, any z)
    float denom = ray_world.y;
    if (std::abs(denom) < 1e-6f) {
        // Ray is parallel to the plane, return camera position as fallback
        return cam_pos;
    }
    float t = (y_offset - cam_pos.y) / denom;
    glm::vec3 world_position = cam_pos + t * ray_world;
    return world_position;
}

