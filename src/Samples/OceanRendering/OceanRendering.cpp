#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"

#include "../../src/Core/VulkanHeader.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

/*
    My pipeline expectation for this project:
        - Draw call of a simple quad formed by 2 triangles
        - Simple vertex shader where we don't even multiply by the MVP matrices
        - Tesselation stage where the quad is split into thousands of triangles and MVP matrices multiplication happens
        - Fragment shader for lighing and color
        - Only noise texture will be used (if used)

    
    Topics to look into and implement:
        - Simpler wave simulation: Gerstner waves (or sine/trochoidal wave)
        - More realistic approach: FFT (Fast Fourier Transform)
        - Normals and micro-detail computation
            - Derive surface normals from wave slopes (either analysitcally from the wave function or from a normal map generated alongside displacement).
        - Water material
            - Fresnel term for view-dependent reflection/refraction.
            - Reflection: Sample from a dynamic reflection probe, cubemap, or rendered reflection texture (planar or screen space)
            - Refraction: Sample scene color with distortion based on normals/wave height (possibly with a separate refraction render target)
            - Color/absorption: Depth-based tinting (deeper water darker/bluer) and subsurface scattering approximation
            - Specular/highlights: Strong sun reflections and environmental specular
            - Foam and details: Add foam on wave crests (based on height/slope)
        - Supporting effects
            - Caustics: Project or render light patterns underwater
            - Interactions: Basic buoyancy for objects, shoreline blending, or particle systems for breaking waves/spray.
            - Environmental integration: match lighting to the sky, add wind direction influence on waves.
        - Optimizations:
            - LOD, Frustum culling, and wave culling for distant areas.
            - Render at lower resolution or with lower simulation resolution far away.
            - Profile compute shaders (FFT can be heavy) and consider multi-threading or pre-computation where possible.
            - Handle horizon clipping and large-world positioning (e.g., grid snapping to camera).
*/

class OceanRendering : public Application::IScene {
public:
	OceanRendering() {
		settings.Title = "OceanRendering.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
	};

	virtual void StartUp()																			override;
	virtual void CleanUp()																			override;
	virtual void Update(const float constantT, const float deltaT, InputSystem::Input& input)		override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer)		override;
	virtual void RenderUI()																			override;
	virtual void Resize(uint32_t width, uint32_t height)											override;

	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 LightPosition;
        alignas(4) float TessellationLevelInner = 64.0f;
        alignas(4) float TessellationLevelOuter = 64.0f;
        alignas(4) float ConstantT = 0.0f;
        alignas(4) float DeltaT = 0.0f;
        alignas(4) float WaveFrequency = 3.0f;
        alignas(4) float WaveAmplitude = 0.01f;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
	} SamplePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(-15.0f, 12.0f, 17.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -52.0f;
	const float InitialCameraPitch	= -33.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

	std::shared_ptr<Assets::Model> m_OceanModel;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_TessellationControlShader = {};
	Graphics::Shader m_TessellationEvaluationShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer = {};

	Graphics::PipelineState m_DefaultPSO = {};
	Graphics::PipelineState m_WireframePSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };

	glm::vec4 m_LightPosition = glm::vec4(1.0f, 1.0f, -15.0f, 1.0f);

	float m_OrbitalLightSpeed = 0.5f;
	float m_OrbitalLightDisplacement = 3.0f;
	bool m_OrbitateLight = false;
    bool m_RenderWireframe = false;
};

void OceanRendering::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));

	Graphics::InputLayout inputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Default";
	desc.vertexShader = &m_VertexShader;
	desc.tessellationControlShader = &m_TessellationControlShader;
	desc.tessellationEvaluationShader = &m_TessellationEvaluationShader;
    desc.tessellationPatchControlPoints = 4;
    desc.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	desc.fragmentShader = &m_FragShader;
	desc.psoInputLayout.push_back(inputLayout);

    gfxDevice->CreatePipelineState(desc, m_DefaultPSO, *m_OffscreenRenderTarget.get());

	desc.Name = "Wireframe";
    desc.lineWidth = 2.0f;
    desc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(desc, m_WireframePSO, *m_OffscreenRenderTarget.get());

	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer);
	}

	m_OceanModel = ModelLoader::LoadModel(ModelType::QUAD);
	m_OceanModel->Transformations.translation.y = -0.51f;
	m_OceanModel->Transformations.rotation.x = 90.0f;
	m_OceanModel->Transformations.scaleHandler = 20.0f;
	m_OceanModel->ModelIndex = 0;
}

void OceanRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_TessellationControlShader);
	gfxDevice->DestroyShader(m_TessellationEvaluationShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_DefaultPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);
}

void OceanRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("OceanRendering::Update");

	m_Camera.OnUpdate(deltaT, input);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_OrbitateLight) {
		m_LightPosition.x = glm::sin(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
		m_LightPosition.z = glm::cos(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
	}

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.LightPosition	= m_LightPosition;
    SampleSceneData.ConstantT       = constantT;
    SampleSceneData.DeltaT          = deltaT;

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
}

void OceanRendering::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("OceanRendering::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
    Graphics::PipelineState* pipeline = m_RenderWireframe ? &m_WireframePSO : &m_DefaultPSO;

	m_OffscreenRenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, pipeline->pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    Assets::Model& Model = *m_OceanModel.get();

    VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
    vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

    SamplePushConstants.Model = Model.GetModelMatrix();

    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

    for (const auto& Mesh: Model.Meshes) {
        // Note: Must use vkCmdDraw instead of vkCmdDrawIndexed to tessellate quads.
        vkCmdDraw(commandBuffer, Mesh.Vertices.size(), Mesh.Indices.size(), 0, 0);
    }

	m_OffscreenRenderTarget->End(commandBuffer);

	m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
}

void OceanRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    ImGui::Checkbox("Render Wireframe",				&m_RenderWireframe);
    // Note: DragFloat signature -> const char *label, float *value, float speed, float min, float max
    ImGui::DragFloat("Tessellation Level Inner",    &SampleSceneData.TessellationLevelInner, 1.0f, 1.0f, 100.0f);
    ImGui::DragFloat("Tessellation Level Outer",    &SampleSceneData.TessellationLevelOuter, 1.0f, 1.0f, 100.0f);
    ImGui::DragFloat("Wave Frequency",              &SampleSceneData.WaveFrequency, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Wave Amplitude",              &SampleSceneData.WaveAmplitude, 0.001f, 0.0f, 10.00f);
	ImGui::Checkbox("Orbitate Light",				&m_OrbitateLight);
	ImGui::DragFloat("Light Orbital Speed",			&m_OrbitalLightSpeed, 0.02f, 0.0f, 3.0f);
	ImGui::DragFloat("Light Orbital Displacement",	&m_OrbitalLightDisplacement, 0.02f, 0.0f, 9.0f);
	ImGui::DragFloat4("Light Position",				(float*)&m_LightPosition, 0.02f, -20.0f, 20.0f);

    m_OceanModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(OceanRendering);
