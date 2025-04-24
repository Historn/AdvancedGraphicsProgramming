//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName, VertexShaderLayout& vertexInputLayout)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};
	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLint attributeCount;
	char attributeName[128];
	GLint attributeNameLength;
	GLint attributeSize;
	GLenum attributeType;
	GLint attributeLocation;

	vertexInputLayout = VertexShaderLayout{};

	glGetProgramiv(programHandle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
	for (u32 i = 0; i < attributeCount; ++i)
	{
		glGetActiveAttrib(programHandle, i,
			ARRAY_COUNT(attributeName),
			&attributeNameLength,
			&attributeSize,
			&attributeType,
			attributeName);

		attributeLocation = glGetAttribLocation(programHandle, attributeName);

		// Store this information in the program's vertexInputLayout
		VertexShaderAttribute attribute;
		attribute.location = attributeLocation;

		// Set componentCount based on attributeType
		switch (attributeType)
		{
		case GL_FLOAT:      attribute.componentCount = 1; break;
		case GL_FLOAT_VEC2: attribute.componentCount = 2; break;
		case GL_FLOAT_VEC3: attribute.componentCount = 3; break;
		case GL_FLOAT_VEC4: attribute.componentCount = 4; break;
		default:            attribute.componentCount = 0; break;
		}

		vertexInputLayout.attributes.push_back(attribute);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName, program.vertexInputLayout);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
	app->programs.push_back(program);

	return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
	Image img = {};
	stbi_set_flip_vertically_on_load(true);
	img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
	if (img.pixels)
	{
		img.stride = img.size.x * img.nchannels;
	}
	else
	{
		ELOG("Could not open file %s", filename);
	}
	return img;
}

void FreeImage(Image image)
{
	stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
	GLenum internalFormat = GL_RGB8;
	GLenum dataFormat = GL_RGB;
	GLenum dataType = GL_UNSIGNED_BYTE;

	switch (image.nchannels)
	{
	case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
	case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
	default: ELOG("LoadTexture2D() - Unsupported number of channels");
	}

	GLuint texHandle;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
	for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
		if (app->textures[texIdx].filepath == filepath)
			return texIdx;

	Image image = LoadImage(filepath);

	if (image.pixels)
	{
		Texture tex = {};
		tex.handle = CreateTexture2DFromImage(image);
		tex.filepath = filepath;

		u32 texIdx = app->textures.size();
		app->textures.push_back(tex);

		FreeImage(image);
		return texIdx;
	}
	else
	{
		return UINT32_MAX;
	}
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	// Try finding a vao for this submesh/program
	for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
	{
		if (submesh.vaos[i].programHandle == program.handle)
		{
			return submesh.vaos[i].handle;
		}
	}

	GLuint vaoHandle = 0;

	// Create a new vao for this submesh/program
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

	// We have to link all vertex inputs attributes to attributes in the vertex buffer
	for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
	{
		bool attributeWasLinked = false;

		for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
		{
			if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
			{
				const u32 index = submesh.vertexBufferLayout.attributes[j].location;
				const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
				const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
				const u32 stride = submesh.vertexBufferLayout.stride;
				glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
				glEnableVertexAttribArray(index);

				attributeWasLinked = true;
				break;
			}
		}
		assert(attributeWasLinked);
	}
	glBindVertexArray(0);

	// Store it in the list of vaos for this submesh
	Vao vao = { vaoHandle, program.handle };
	submesh.vaos.push_back(vao);

	return vaoHandle;
}

void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
	std::vector<float> vertices;
	std::vector<u32> indices;

	bool hasTexCoords = false;
	bool hasTangentSpace = false;

	// process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		vertices.push_back(mesh->mVertices[i].x);
		vertices.push_back(mesh->mVertices[i].y);
		vertices.push_back(mesh->mVertices[i].z);
		vertices.push_back(mesh->mNormals[i].x);
		vertices.push_back(mesh->mNormals[i].y);
		vertices.push_back(mesh->mNormals[i].z);

		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			hasTexCoords = true;
			vertices.push_back(mesh->mTextureCoords[0][i].x);
			vertices.push_back(mesh->mTextureCoords[0][i].y);
		}

		if (mesh->mTangents != nullptr && mesh->mBitangents)
		{
			hasTangentSpace = true;
			vertices.push_back(mesh->mTangents[i].x);
			vertices.push_back(mesh->mTangents[i].y);
			vertices.push_back(mesh->mTangents[i].z);

			// For some reason ASSIMP gives me the bitangents flipped.
			// Maybe it's my fault, but when I generate my own geometry
			// in other files (see the generation of standard assets)
			// and all the bitangents have the orientation I expect,
			// everything works ok.
			// I think that (even if the documentation says the opposite)
			// it returns a left-handed tangent space matrix.
			// SOLUTION: I invert the components of the bitangent here.
			vertices.push_back(-mesh->mBitangents[i].x);
			vertices.push_back(-mesh->mBitangents[i].y);
			vertices.push_back(-mesh->mBitangents[i].z);
		}
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// store the proper (previously proceessed) material for this mesh
	submeshMaterialIndices.push_back(baseMeshMaterialIndex + mesh->mMaterialIndex);

	// create the vertex format
	VertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
	vertexBufferLayout.stride = 6 * sizeof(float);
	if (hasTexCoords)
	{
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 2 * sizeof(float);
	}
	if (hasTangentSpace)
	{
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);

		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);
	}

	// add the submesh into the mesh
	Submesh submesh = {};
	submesh.vertexBufferLayout = vertexBufferLayout;
	submesh.vertices.swap(vertices);
	submesh.indices.swap(indices);
	myMesh->submeshes.push_back(submesh);
}

void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory)
{
	aiString name;
	aiColor3D diffuseColor;
	aiColor3D emissiveColor;
	aiColor3D specularColor;
	ai_real shininess;
	material->Get(AI_MATKEY_NAME, name);
	material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
	material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
	material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
	material->Get(AI_MATKEY_SHININESS, shininess);

	myMaterial.name = name.C_Str();
	myMaterial.albedo = vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	myMaterial.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	myMaterial.smoothness = shininess / 256.0f;

	aiString aiFilename;
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		material->GetTexture(aiTextureType_DIFFUSE, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.albedoTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
	{
		material->GetTexture(aiTextureType_EMISSIVE, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.emissiveTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
	{
		material->GetTexture(aiTextureType_SPECULAR, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.specularTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
	{
		material->GetTexture(aiTextureType_NORMALS, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.normalsTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
	{
		material->GetTexture(aiTextureType_HEIGHT, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.bumpTextureIdx = LoadTexture2D(app, filepath.str);
	}

	//myMaterial.createNormalFromBump();
}

void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessAssimpMesh(scene, mesh, myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
	}

	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessAssimpNode(scene, node->mChildren[i], myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
	}
}

u32 LoadModel(App* app, const char* filename)
{
	const aiScene* scene = aiImportFile(filename,
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_PreTransformVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_OptimizeMeshes |
		aiProcess_SortByPType);

	if (!scene)
	{
		ELOG("Error loading mesh %s: %s", filename, aiGetErrorString());
		return UINT32_MAX;
	}

	app->meshes.push_back(Mesh{});
	Mesh& mesh = app->meshes.back();
	u32 meshIdx = (u32)app->meshes.size() - 1u;

	app->models.push_back(Model{});
	Model& model = app->models.back();
	model.meshIdx = meshIdx;
	u32 modelIdx = (u32)app->models.size() - 1u;

	String directory = GetDirectoryPart(MakeString(filename));

	// Create a list of materials
	u32 baseMeshMaterialIndex = (u32)app->materials.size();
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		app->materials.push_back(Material{});
		Material& material = app->materials.back();
		ProcessAssimpMaterial(app, scene->mMaterials[i], material, directory);
	}

	ProcessAssimpNode(scene, scene->mRootNode, &mesh, baseMeshMaterialIndex, model.materialIdx);

	aiReleaseImport(scene);

	u32 vertexBufferSize = 0;
	u32 indexBufferSize = 0;

	for (u32 i = 0; i < mesh.submeshes.size(); ++i)
	{
		vertexBufferSize += mesh.submeshes[i].vertices.size() * sizeof(float);
		indexBufferSize += mesh.submeshes[i].indices.size() * sizeof(u32);
	}

	glGenBuffers(1, &mesh.vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &mesh.indexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

	u32 indicesOffset = 0;
	u32 verticesOffset = 0;

	for (u32 i = 0; i < mesh.submeshes.size(); ++i)
	{
		const void* verticesData = mesh.submeshes[i].vertices.data();
		const u32   verticesSize = mesh.submeshes[i].vertices.size() * sizeof(float);
		glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
		mesh.submeshes[i].vertexOffset = verticesOffset;
		verticesOffset += verticesSize;

		const void* indicesData = mesh.submeshes[i].indices.data();
		const u32   indicesSize = mesh.submeshes[i].indices.size() * sizeof(u32);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
		mesh.submeshes[i].indexOffset = indicesOffset;
		indicesOffset += indicesSize;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return modelIdx;
}

void Init(App* app)
{
	// TODO: Initialize your resources here!
	// - vertex buffers
	// - element/index buffers
	// - vaos
	// - programs (and retrieve uniform indices)
	// - textures

	if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.major >= 3))
	{
		//glDebugMessageCallback(OnGlError, app);
	}

	InitGUI(app);

	// Initialize transformation matrices
	app->modelMatrix = glm::mat4(1.0f); // Identity matrix

	// Set up camera
	app->cameraPosition = glm::vec3(0.0f, 1.0f, 8.0f);
	app->cameraTarget = glm::vec3(0.0f, 1.0f, 0.0f);
	app->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	app->cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	app->cameraFov = 90.0f;

	// Create view matrix
	app->viewMatrix = glm::lookAt(app->cameraPosition, app->cameraTarget, app->cameraUp);

	// Create projection matrix
	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	app->projectionMatrix = glm::perspective(glm::radians(app->cameraFov), aspectRatio, 0.1f, 100.0f);

	app->model = LoadModel(app, "Patrick/Patrick.obj");

	// Create an entity for the model and add it to entities collection
	Entity entity;
	entity.modelIdx = app->model;
	entity.transform = app->modelMatrix; // Use the model matrix as the initial transform
	app->entities.push_back(entity);

	app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH"); // Name established also in .glsl file
	Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
	app->modelMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uModel");
	app->viewMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uView");
	app->projectionMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uProjection");
	app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

	app->mode = Mode_TexturedMesh;
}

void InitGUI(App* app)
{
	// Init Combo values for selecting render pass
	const char* items[] = { "Final", "Albedo", "Normals", "Position", "Depth" };
	for (u32 i = 0; i < 5; i++)
	{
		app->renderpasses[i] = items[i];
	}
}

void Gui(App* app)
{
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	// Add transformation controls
	static float position[3] = { 0.0f, 0.0f, 0.0f };
	static float rotation[3] = { 0.0f, 0.0f, 0.0f };
	static float scale[3] = { 1.0f, 1.0f, 1.0f };

	// Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
	const char* combo_preview_value = app->renderpasses[app->renderpass_selected];
	ImGuiComboFlags flags = 0;
	if (ImGui::BeginCombo("Render Passes", combo_preview_value, flags))
	{
		for (int n = 0; n < 5; n++)
		{
			const bool is_selected = (app->renderpass_selected == n);
			if (ImGui::Selectable(app->renderpasses[n], is_selected))
				app->renderpass_selected = n;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// CHANGE FOR ALL THE ENTITIES HANDLING
	if (ImGui::SliderFloat3("Position", position, -10.0f, 10.0f) || ImGui::SliderFloat3("Rotation", rotation, 0.0f, 360.0f) ||
		ImGui::SliderFloat3("Scale", scale, 0.1f, 5.0f))
	{
		// Same transformation code as above
		app->modelMatrix = glm::mat4(1.0f);
		app->modelMatrix = glm::translate(app->modelMatrix, glm::vec3(position[0], position[1], position[2]));
		app->modelMatrix = glm::rotate(app->modelMatrix, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
		app->modelMatrix = glm::rotate(app->modelMatrix, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
		app->modelMatrix = glm::rotate(app->modelMatrix, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));
		app->modelMatrix = glm::scale(app->modelMatrix, glm::vec3(scale[0], scale[1], scale[2]));
	}
	ImGui::End();
}

void CameraMovement(App* app)
{
	f32 cameraSpeed = 5.0f * app->deltaTime;
	// Camera controls
	if (app->input.keys[K_W])
	{
		app->cameraPosition += cameraSpeed * app->cameraFront;
		app->viewMatrix = glm::lookAt(app->cameraPosition, app->cameraPosition + app->cameraFront, app->cameraUp);
	}
	if (app->input.keys[K_S])
	{
		app->cameraPosition -= cameraSpeed * app->cameraFront;
		app->viewMatrix = glm::lookAt(app->cameraPosition, app->cameraPosition + app->cameraFront, app->cameraUp);
	}
	if (app->input.keys[K_D])
	{
		app->cameraPosition += glm::normalize(glm::cross(app->cameraFront, app->cameraUp)) * cameraSpeed;
		app->viewMatrix = glm::lookAt(app->cameraPosition, app->cameraPosition + app->cameraFront, app->cameraUp);
	}
	if (app->input.keys[K_A])
	{
		app->cameraPosition -= glm::normalize(glm::cross(app->cameraFront, app->cameraUp)) * cameraSpeed;
		app->viewMatrix = glm::lookAt(app->cameraPosition, app->cameraPosition + app->cameraFront, app->cameraUp);
	}
}

void Update(App* app)
{
	for (u64 i = 0; i < app->programs.size(); ++i)
	{
		Program& program = app->programs[i];
		u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
		if (currentTimestamp > program.lastWriteTimestamp)
		{
			glDeleteProgram(program.handle);
			String programSource = ReadTextFile(program.filepath.c_str());
			const char* programName = program.programName.c_str();
			program.handle = CreateProgramFromSource(programSource, programName, program.vertexInputLayout);
			program.lastWriteTimestamp = currentTimestamp;
		}
	}

	// Rotate model around Y axis
	/*float rotationSpeed = 1.0f;
	app->modelMatrix = glm::rotate(app->modelMatrix, app->deltaTime * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
	*/

	// NOT SUPPOSED TO SET THE MODEL MATRIX, SET IN INIT AND CHANGE IT USING THE TRANSFORMATIONS
	if (!app->entities.empty()) {
		app->entities[0].transform = app->modelMatrix;
	}
}

void Render(App* app)
{
	switch (app->mode)
	{
	case Mode_TexturedQuad:
	{
		
	}
	case Mode_TexturedMesh:
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
		glUseProgram(texturedMeshProgram.handle);

		// Update projection matrix in case window was resized
		float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
		app->projectionMatrix = glm::perspective(glm::radians(app->cameraFov), aspectRatio, 0.1f, 100.0f);

		// Pass matrices to shader
		glUniformMatrix4fv(app->viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(app->viewMatrix));
		glUniformMatrix4fv(app->projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(app->projectionMatrix));

		for (const Entity& entity : app->entities)
		{
			glUniformMatrix4fv(app->modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(entity.transform));

			Model& entityModel = app->models[entity.modelIdx];
			Mesh& entityMesh = app->meshes[entityModel.meshIdx];

			for (u32 i = 0; i < entityMesh.submeshes.size(); ++i)
			{
				GLuint vao = FindVAO(entityMesh, i, texturedMeshProgram);
				glBindVertexArray(vao);

				u32 submeshMaterialIdx = entityModel.materialIdx[i];
				Material& submeshMaterial = app->materials[submeshMaterialIdx];

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
				glUniform1i(app->texturedMeshProgram_uTexture, 0);

				Submesh& submesh = entityMesh.submeshes[i];
				glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
			}
		}

		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(0);
		glUseProgram(0);
	}
	break;

	case Mode_Forward:
	{

	}
	break;

	case Mode_Deferred:
	{

	}
	break;

	default:;
	}
}

