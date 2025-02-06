#include <memory>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"
#include "../Core/RenderTarget.h"
#include "../Core/ResourceManager.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"

#include "../Utils/ModelLoader.h"

class Geometry : public Application::IScene {
public:
	Geometry() {
		settings.Title			= "Geomtry Shader.exe";
		settings.Width			= 1600;
		settings.Height			= 800;
		settings.uiEnabled		= true;
	}

	virtual void StartUp()																		override;
	virtual void CleanUp()																		override;
	virtual void Update(const float constantT, const float deltaT, InputSystem::Input& input)	override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI()																		override;
	virtual void Resize(uint32_t width, uint32_t height)										override;
private:

	struct PushConstant {
		int materialIndex;
	} m_PushConstant;

	struct SceneGPUData {
		float time			= 0.0f;					// 4
		float explode		= 0.0f;					// 8
		int extra_2			= 0;					// 12
		int extra_3			= 0;					// 16
		glm::vec4 extra[7]	= {};					// 128
		glm::mat4 view		= glm::mat4(1.0f);		// 192
		glm::mat4 proj		= glm::mat4(1.0f);		// 256
	} m_SceneGPUData;

	struct ModelGPUData {
		int flip_uv_vertically	= 0;				// 4
		int extra_1				= 0;				// 8
		int extra_2				= 0;				// 12
		int extra_3				= 0;				// 16
		glm::vec4 extra[7]		= {};				// 128
		glm::mat4 model			= glm::mat4(1.0f);	// 192
		glm::mat4 normal		= glm::mat4(1.0f);	// 256
	} m_ModelGPUData;

	std::shared_ptr<Assets::Model>	m_Model;

	uint32_t						m_Width				= 0;
	uint32_t						m_Height			= 0;

	bool							m_ExplodeByTime		= false;
	bool							m_ManualExplode		= false;

	Assets::Camera					m_Camera			= {};

	Graphics::Buffer				m_SceneDataBuffer	= {};
	Graphics::Buffer				m_ModelDataBuffer	= {};
	Graphics::Shader				m_VertexShader		= {};
	Graphics::Shader				m_GeometryShader	= {};
	Graphics::Shader				m_FragmentShader	= {};
	Graphics::PipelineState			m_PSO				= {};
	Graphics::InputLayout			m_PSOInputLayout	= {};

	VkDescriptorSet					m_Set				= VK_NULL_HANDLE;
};

void Geometry::StartUp() {
	m_Width		= settings.Width;
	m_Height	= settings.Height;


	m_Model									= ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj");
	m_Model->Transformations.scaleHandler	= 0.3f;
	m_Model->FlipUvVertically				= true;

	m_Camera.Init(glm::vec3(0.0f, 0.0f, 5.0f), 45.0f, 270.0f, 0.0f, m_Width, m_Height);
	
	ResourceManager* rm = ResourceManager::Get();

	m_PSOInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstant) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT },								// Scene GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },															// Model GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},															// Material Data
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT }		// Textures Array 
		}
	};

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"../src/Samples/Geometry Shader/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"../src/Samples/Geometry Shader/fragment.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_GEOMETRY_BIT, m_GeometryShader,	"../src/Samples/Geometry Shader/geometry.glsl");
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"shader_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"shader_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_GEOMETRY_BIT, m_GeometryShader,	"shader_geom.spv");
#endif

	m_SceneDataBuffer = gfxDevice->CreateBuffer(sizeof(SceneGPUData));
	m_ModelDataBuffer = gfxDevice->CreateBuffer(sizeof(ModelGPUData));
	
	Graphics::PipelineStateDescription desc = {};
	desc.Name								= "Color Pipeline";
	desc.vertexShader						= &m_VertexShader;
	desc.fragmentShader						= &m_FragmentShader;
	desc.geometryShader						= &m_GeometryShader;
	desc.cullMode							= VK_CULL_MODE_NONE;
	desc.psoInputLayout						.push_back(m_PSOInputLayout);

	gfxDevice->CreatePipelineState(desc, m_PSO, *gfxDevice->GetSwapChain().RenderTarget.get());
	gfxDevice->CreateDescriptorSet(m_PSO.descriptorSetLayout, m_Set);
	gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[0], m_Set, m_SceneDataBuffer);
	gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[1], m_Set, m_ModelDataBuffer);
	gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[2], m_Set, rm->GetMaterialBuffer());
	gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[3], m_Set, rm->GetTextures());
}

void Geometry::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_GeometryShader);
	gfxDevice->DestroyShader(m_FragmentShader);
	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyPipeline(m_PSO);

	m_Model.reset();
}

void Geometry::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	m_Camera.OnUpdate(deltaT, input);
	m_Model->OnUpdate(deltaT);

	if (!m_ExplodeByTime)
		m_SceneGPUData.time	= 0.0f;
	else
		m_SceneGPUData.time	= constantT;

	m_SceneGPUData.view					= m_Camera.ViewMatrix;
	m_SceneGPUData.proj					= m_Camera.ProjectionMatrix;

	m_ModelGPUData.flip_uv_vertically	= m_Model->FlipUvVertically;
	m_ModelGPUData.model				= m_Model->GetModelMatrix();
	m_ModelGPUData.normal				= glm::mat4(glm::mat3(glm::transpose(glm::inverse(m_ModelGPUData.model))));

	gfxDevice->UpdateBuffer(m_SceneDataBuffer, &m_SceneGPUData);
	gfxDevice->UpdateBuffer(m_ModelDataBuffer, &m_ModelGPUData);
}

void Geometry::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->GetSwapChain().RenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_Set, commandBuffer, m_PSO.pipelineLayout, 0, 1);

	VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Model->TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Model->DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (const auto& mesh : m_Model->Meshes) {

		m_PushConstant.materialIndex = static_cast<int>(mesh.MaterialIndex);

		vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstant), &m_PushConstant);

		vkCmdDrawIndexed(
			commandBuffer,
			static_cast<uint32_t>(mesh.Indices.size()),
			1,
			static_cast<uint32_t>(mesh.IndexOffset),
			static_cast<int32_t>(mesh.VertexOffset),
			0
		);
	}

	// gfxDevice->GetSwapChain().RenderTarget->End(commandBuffer); -> When using swap chain render target it should not be ended here
}

void Geometry::RenderUI() {
	m_Camera.OnUIRender("Main Camera - Settings");
	m_Model->OnUIRender();

	ImGui::Text("Time: %f", m_SceneGPUData.time);

	ImGui::Checkbox("Time (Explode Effect)", &m_ExplodeByTime);
	ImGui::Checkbox("Manual (Explode Effect)", &m_ManualExplode);

	if (m_ManualExplode)
		ImGui::DragFloat("Amount", &m_SceneGPUData.explode, 0.002f, -1.0f, 1.0f);
}

void Geometry::Resize(uint32_t width, uint32_t height) {
	m_Width		= width;
	m_Height	= height;
}

RUN_APPLICATION(Geometry);
