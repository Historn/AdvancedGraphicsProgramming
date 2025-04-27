//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Transform
{
	vec3 position;
	vec3 rotation;
	vec3 scale;
	glm::mat4 transformMatrix;

	void UpdateTransformations()
	{
		position = glm::vec3(transformMatrix[3]);
		glm::extractEulerAngleXYZ(transformMatrix, rotation.x, rotation.y, rotation.z);
		scale.x = glm::length(glm::vec3(transformMatrix[0]));
		scale.y = glm::length(glm::vec3(transformMatrix[1]));
		scale.z = glm::length(glm::vec3(transformMatrix[2]));
	}
};

struct Camera
{
	vec3 position;
	vec3 target;
	vec3 up;
	vec3 front;
	float fov;
	float zNear;
	float zFar;
	f32 pitch;
	f32 yaw;
};

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 color;
	float intensity;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec3 color;
	float intensity;
	float radius;
};

struct Entity
{
	const char* name;
	u32 modelIdx;
	Transform transform;
};

struct VertexBufferAttribute
{
	u8 location;
	u8 componentCount;
	u8 offset;
};

struct VertexBufferLayout
{
	std::vector<VertexBufferAttribute> attributes;
	u8 stride;
};

struct VertexShaderAttribute
{
	u8 location;
	u8 componentCount;
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};

struct Image
{
	void* pixels;
	ivec2 size;
	i32   nchannels;
	i32   stride;
};

struct Texture
{
	GLuint      handle;
	std::string filepath;
};

struct Model
{
	u32 meshIdx;
	std::vector<u32> materialIdx;
};

struct Submesh
{
	VertexBufferLayout vertexBufferLayout;
	std::vector<float> vertices;
	std::vector<u32> indices;
	u32 vertexOffset;
	u32 indexOffset;
	std::vector<Vao> vaos;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	GLuint vertexBufferHandle;
	GLuint indexBufferHandle;
};

struct Material
{
	std::string name;
	vec3 albedo;
	vec3 emissive;
	f32 smoothness;
	u32 albedoTextureIdx;
	u32 emissiveTextureIdx;
	u32 specularTextureIdx;
	u32 normalsTextureIdx;
	u32 bumpTextureIdx;
	// Add shader
};

struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp;
	VertexShaderLayout vertexInputLayout;
};

enum Mode
{
	Mode_TexturedMesh,
	Mode_Deferred,
	Mode_DebugGBuffer,
	Mode_Count
};

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

struct GBuffer
{
	GLuint fbo;
	GLuint albedoTexture;
	GLuint normalTexture;
	GLuint positionTexture;
	GLuint depthTexture;
};

struct App
{
	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;

	// Graphics
	char gpuName[64];
	char openGlVersion[64];

	ivec2 displaySize;
	ivec2 lastDisplaySize;

	std::vector<Texture>    textures;
	std::vector<Material>   materials;
	std::vector<Mesh>       meshes;
	std::vector<Model>      models;
	std::vector<Program>    programs;

	// Programs indices
	u32 texturedMeshProgramIdx;
	u32 geometryPassProgramIdx;
	u32 deferredLightingProgramIdx;
	u32 debugProgramIdx;

	// Mode
	Mode mode;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;

	// Matrices for transformations
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

#pragma region TexturedMeshRendering
	// Textured Mesh uniform locations
	GLuint modelMatrixLocation;
	GLuint viewMatrixLocation;
	GLuint projectionMatrixLocation;
	GLuint texturedMeshProgram_uTexture;
#pragma endregion

#pragma region DeferredRendering
	/*DEFERRED SHADING*/
	GBuffer gBuffer;

	// GPass Uniform locations
	GLuint geometryModelLoc;
	GLuint geometryViewLoc;
	GLuint geometryProjLoc;
	GLuint geometryTexLoc;

	// Locations for deferred lighting shader uniforms
	GLuint lightingNumDirLightsLoc;
	GLuint lightingNumPointLightsLoc;
	GLuint lightingAlbedoTexLoc;
	GLuint lightingNormalTexLoc;
	GLuint lightingPositionTexLoc;
	GLuint lightingDepthTexLoc;
	GLuint lightPositionLoc;
	GLuint viewPosLoc;
#pragma endregion

#pragma region DebugGBufferRendering
	// Debug GBuffer uniform locations
	GLuint debugTextureLoc;
	GLuint debugTexTypeLoc;
	GLuint debugZnearLoc;
	GLuint debugZfarLoc;
#pragma endregion

#pragma region Lights
	// Lights
	std::vector<DirectionalLight> dirLights;
	std::vector<PointLight> pointLights;

	// Light visualization
	u32 lightSphereProgramIdx;
	u32 lightSphereVAO;
	u32 lightSphereVBO;
	u32 lightSphereEBO;
	int lightSphereIndexCount;
#pragma endregion

	// Entities
	std::vector<Entity> entities;
	u32 selectedEntity;

	// Camera
	Camera camera;

	// Quad
	GLuint quadVBO;

	// GUI
	int renderpass_selected = 0;
};

void Init(App* app);

#pragma region Initializers
/*Initializers*/
void InitCamera(App* app);

void InitLights(App* app);

void InitGBufferRendering(App* app);

void InitDeferredRendering(App* app);

void InitTexturedMeshRendering(App* app);

void InitDebugGBufferRendering(App* app);

void InitQuad(App* app);
/*Initializers*/
#pragma endregion

void Gui(App* app);

void CameraMovement(App* app);

void ResizeGBuffer(App* app);

void Update(App* app);

void Render(App* app);

void CleanUp(App* app);

