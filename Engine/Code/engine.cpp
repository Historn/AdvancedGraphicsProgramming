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

void LoadEntity(App* app, const char* modelFilename, const char* entityName)
{
	Entity entity;
	entity.name = entityName;
	entity.modelIdx = LoadModel(app, modelFilename);
	entity.transform.transformMatrix = glm::mat4(1.0f); // Identity matrix
	entity.transform.UpdateTransformations();
	app->entities.push_back(entity);
}

void CreateLightSphere(App* app)
{
	const int segments = 16;
	const int rings = 16;
	const float radius = 1.0f;

	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> indices;

	// Generate vertices
	for (int i = 0; i <= rings; ++i)
	{
		float phi = glm::pi<float>() * i / rings;
		for (int j = 0; j <= segments; ++j)
		{
			float theta = 2.0f * glm::pi<float>() * j / segments;

			float x = radius * sin(phi) * cos(theta);
			float y = radius * sin(phi) * sin(theta);
			float z = radius * cos(phi);

			vertices.push_back(glm::vec3(x, y, z));
		}
	}

	// Generate indices
	for (int i = 0; i < rings; ++i)
	{
		for (int j = 0; j < segments; ++j)
		{
			int first = (i * (segments + 1)) + j;
			int second = first + (segments + 1);

			indices.push_back(first);
			indices.push_back(second);
			indices.push_back(first + 1);

			indices.push_back(second);
			indices.push_back(second + 1);
			indices.push_back(first + 1);
		}
	}

	app->lightSphereIndexCount = (int)indices.size();

	// Create VAO, VBO and EBO
	glGenVertexArrays(1, &app->lightSphereVAO);
	glGenBuffers(1, &app->lightSphereVBO);
	glGenBuffers(1, &app->lightSphereEBO);

	glBindVertexArray(app->lightSphereVAO);

	glBindBuffer(GL_ARRAY_BUFFER, app->lightSphereVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->lightSphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindVertexArray(0);
}

void Init(App* app)
{
	if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.major >= 3))
	{
		//glDebugMessageCallback(OnGlError, app);
	}

	// Set up camera
	InitCamera(app);

	InitLights(app);

	// Create view matrix
	app->viewMatrix = glm::lookAt(app->camera.position, app->camera.target, app->camera.up);

	// Create projection matrix
	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	app->projectionMatrix = glm::perspective(glm::radians(app->camera.fov), aspectRatio, app->camera.zNear, app->camera.zFar);

	LoadEntity(app, "Patrick/Patrick.obj", "Patricio 1");
	LoadEntity(app, "Patrick/Patrick.obj", "Patricio 2");
	LoadEntity(app, "Patrick/Patrick.obj", "Patricio 3");
	app->selectedEntity = 0;

	InitDeferredRendering(app);

	app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH"); // Name established also in .glsl file
	Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
	app->modelMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uModel");
	app->viewMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uView");
	app->projectionMatrixLocation = glGetUniformLocation(texturedMeshProgram.handle, "uProjection");
	app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

	InitQuad(app);

	// Set actual shading model
	app->mode = Mode_TexturedMesh;
	app->renderpass_selected = 0;
}

void InitCamera(App* app)
{
	app->camera.position = glm::vec3(0.0f, 1.0f, 8.0f);
	app->camera.target = glm::vec3(0.0f, 1.0f, 0.0f);
	app->camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	app->camera.front = glm::vec3(0.0f, 0.0f, -1.0f);

	app->camera.fov = 90.0f;
	app->camera.zNear = 0.1f;
	app->camera.zFar = 1000.0f;

	app->camera.yaw = -90.0f;
	app->camera.pitch = 0.0f;
}

void InitLights(App* app)
{
	// Create light visualization geometry
	CreateLightSphere(app);

	// Directional lights
	DirectionalLight dirLight1;
	dirLight1.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
	dirLight1.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight1.intensity = 1.0f;
	app->dirLights.push_back(dirLight1);

	DirectionalLight dirLight2;
	dirLight2.direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.5f));
	dirLight2.color = glm::vec3(0.8f, 0.8f, 1.0f);
	dirLight2.intensity = 0.5f;
	app->dirLights.push_back(dirLight2);

	// Point lights
	PointLight pointLight1;
	pointLight1.position = glm::vec3(2.0f, 1.0f, 2.0f);
	pointLight1.color = glm::vec3(1.0f, 0.5f, 0.5f);
	pointLight1.intensity = 2.0f;
	pointLight1.radius = 5.0f;
	app->pointLights.push_back(pointLight1);

	PointLight pointLight2;
	pointLight2.position = glm::vec3(-2.0f, 1.0f, -2.0f);
	pointLight2.color = glm::vec3(0.5f, 1.0f, 0.5f);
	pointLight2.intensity = 2.0f;
	pointLight2.radius = 5.0f;
	app->pointLights.push_back(pointLight2);

	PointLight pointLight3;
	pointLight3.position = glm::vec3(0.0f, 1.0f, -3.0f);
	pointLight3.color = glm::vec3(0.5f, 0.5f, 1.0f);
	pointLight3.intensity = 2.0f;
	pointLight3.radius = 5.0f;
	app->pointLights.push_back(pointLight3);
}

void InitDeferredRendering(App* app)
{
	// Create G-Buffer
	glGenFramebuffers(1, &app->gBuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer.fbo);

	// Create textures for G-Buffer
	// Albedo texture
	glGenTextures(1, &app->gBuffer.albedoTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.albedoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->gBuffer.albedoTexture, 0);

	// Normal texture
	glGenTextures(1, &app->gBuffer.normalTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.normalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->gBuffer.normalTexture, 0);

	// Position texture
	glGenTextures(1, &app->gBuffer.positionTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.positionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, app->gBuffer.positionTexture, 0);

	// Depth texture
	glGenTextures(1, &app->gBuffer.depthTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->gBuffer.depthTexture, 0);

	// Tell OpenGL which color attachments we'll use for rendering
	GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		ELOG("Framebuffer not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Load geometry pass
	app->geometryPassProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_PASS");
	Program& geometryPassProgram = app->programs[app->geometryPassProgramIdx];
	app->geometryModelLoc = glGetUniformLocation(geometryPassProgram.handle, "uModel");
	app->geometryViewLoc = glGetUniformLocation(geometryPassProgram.handle, "uView");
	app->geometryProjLoc = glGetUniformLocation(geometryPassProgram.handle, "uProjection");
	app->geometryTexLoc = glGetUniformLocation(geometryPassProgram.handle, "uTexture");

	// Load deferred lighting shader
	app->gBuffer.deferredLightingProgramIdx = LoadProgram(app, "shaders.glsl", "DEFERRED_LIGHTING");
	Program& lightingProgram = app->programs[app->gBuffer.deferredLightingProgramIdx];

	// Get uniform locations
	app->lightingAlbedoTexLoc = glGetUniformLocation(lightingProgram.handle, "uAlbedoTexture");
	app->lightingNormalTexLoc = glGetUniformLocation(lightingProgram.handle, "uNormalTexture");
	app->lightingPositionTexLoc = glGetUniformLocation(lightingProgram.handle, "uPositionTexture");
	app->lightingDepthTexLoc = glGetUniformLocation(lightingProgram.handle, "uDepthTexture");
	app->lightPositionLoc = glGetUniformLocation(lightingProgram.handle, "uLightPosition");
	app->viewPosLoc = glGetUniformLocation(lightingProgram.handle, "uViewPos");
}

void InitQuad(App* app)
{
	// Quad vertices
	float quadVertices[] =
	{
		// position        // texCoords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	glGenVertexArrays(1, &app->vao);
	glGenBuffers(1, &app->quadVBO); // Add quadVBO to App struct
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Gui(App* app)
{
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);

	// Add mode selection
	const char* modes[] = { "Textured Mesh", "Deferred Shading" }; // By now these supported
	int currentMode = app->mode;
	if (ImGui::Combo("Rendering Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
	{
		app->mode = (Mode)currentMode;
	}

	if (app->mode == Mode_Deferred)
	{
		const char* renderpasses[] = { "Final", "Albedo", "Normals", "Position", "Depth" };
		const char* combo_value = renderpasses[app->renderpass_selected];
		ImGuiComboFlags flags = 0;
		if (ImGui::BeginCombo("Render Passes", combo_value, flags))
		{
			for (int n = 0; n < 5; n++)
			{
				const bool is_selected = (app->renderpass_selected == n);
				if (ImGui::Selectable(renderpasses[n], is_selected))
					app->renderpass_selected = n;


				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Light controls
		if (ImGui::TreeNode("Directional Lights"))
		{
			for (size_t i = 0; i < app->dirLights.size(); ++i)
			{
				if (ImGui::TreeNode((void*)(intptr_t)i, "Directional Light %d", (int)i))
				{
					ImGui::DragFloat3("Direction", &app->dirLights[i].direction.x, 0.01f);
					ImGui::ColorEdit3("Color", &app->dirLights[i].color.x);
					ImGui::DragFloat("Intensity", &app->dirLights[i].intensity, 0.01f, 0.0f, 10.0f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Point Lights"))
		{
			for (size_t i = 0; i < app->pointLights.size(); ++i)
			{
				if (ImGui::TreeNode((void*)(intptr_t)i, "Point Light %d", (int)i))
				{
					ImGui::DragFloat3("Position", &app->pointLights[i].position.x, 0.1f);
					ImGui::ColorEdit3("Color", &app->pointLights[i].color.x);
					ImGui::DragFloat("Intensity", &app->pointLights[i].intensity, 0.01f, 0.0f, 10.0f);
					ImGui::DragFloat("Radius", &app->pointLights[i].radius, 0.1f, 0.1f, 20.0f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}

	// Transformation controls in float arrays for ImGUI dragfloat
	float position[3] = { app->entities[app->selectedEntity].transform.position.x, app->entities[app->selectedEntity].transform.position.y, app->entities[app->selectedEntity].transform.position.z };
	float rotation[3] = { app->entities[app->selectedEntity].transform.rotation.x, app->entities[app->selectedEntity].transform.rotation.y, app->entities[app->selectedEntity].transform.rotation.z };
	float scale[3] = { app->entities[app->selectedEntity].transform.scale.x, app->entities[app->selectedEntity].transform.scale.y, app->entities[app->selectedEntity].transform.scale.z };

	if (ImGui::DragFloat3("Position", position) || ImGui::DragFloat3("Rotation", rotation) ||
		ImGui::DragFloat3("Scale", scale))
	{
		glm::mat4* mat = &app->entities[app->selectedEntity].transform.transformMatrix;
		*mat = glm::mat4(1.0f);
		*mat = glm::translate(*mat, glm::vec3(position[0], position[1], position[2]));
		*mat = glm::rotate(*mat, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
		*mat = glm::rotate(*mat, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
		*mat = glm::rotate(*mat, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));
		*mat = glm::scale(*mat, glm::vec3(scale[0], scale[1], scale[2]));
		app->entities[app->selectedEntity].transform.UpdateTransformations();
	}
	ImGui::End();

	// Entities List Window
	ImGui::Begin("Entities");
	ImGui::BeginChild("Entities", ImVec2(200.0f, 200.0f), 0, 0);
	for (u32 i = 0; i < app->entities.size(); i++)
	{
		const bool is_selected = (app->selectedEntity == i);
		if (ImGui::Selectable(app->entities[i].name, is_selected))
			app->selectedEntity = i;
	}
	ImGui::EndChild();
	ImGui::End();
}

void CameraMovement(App* app)
{
	bool updateCamera = false;
	vec3 movement(0.0f);
	f32 cameraSpeed = 5.0f * app->deltaTime;

	if (app->input.mouseButtons[1])
	{
		f32 sensitivity = 10.0f * app->deltaTime;
		app->input.mouseDelta.x *= sensitivity;
		app->input.mouseDelta.y *= sensitivity;

		app->camera.yaw += app->input.mouseDelta.x;
		app->camera.pitch -= app->input.mouseDelta.y;

		app->camera.pitch = glm::clamp(app->camera.pitch, -89.0f, 89.0f);

		vec3 direction;
		direction.x = cos(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		direction.y = sin(glm::radians(app->camera.pitch));
		direction.z = sin(glm::radians(app->camera.yaw)) * cos(glm::radians(app->camera.pitch));
		app->camera.front = glm::normalize(direction);
		app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}

	if (app->input.keys[K_W])
	{
		movement += cameraSpeed * app->camera.front;
		updateCamera = true;
		//app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}
	if (app->input.keys[K_S])
	{
		movement -= cameraSpeed * app->camera.front;
		updateCamera = true;
		//app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}
	if (app->input.keys[K_D])
	{
		movement += glm::normalize(glm::cross(app->camera.front, app->camera.up)) * cameraSpeed;
		updateCamera = true;
		//app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}
	if (app->input.keys[K_A])
	{
		movement -= glm::normalize(glm::cross(app->camera.front, app->camera.up)) * cameraSpeed;
		updateCamera = true;
		//app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}

	if (updateCamera)
	{
		app->camera.position += movement;
		app->viewMatrix = glm::lookAt(app->camera.position, app->camera.position + app->camera.front, app->camera.up);
	}
}

void ResizeGBuffer(App* app)
{
	// Delete old textures
	glDeleteTextures(1, &app->gBuffer.albedoTexture);
	glDeleteTextures(1, &app->gBuffer.normalTexture);
	glDeleteTextures(1, &app->gBuffer.positionTexture);
	glDeleteTextures(1, &app->gBuffer.depthTexture);

	// Create new textures with updated size
	glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer.fbo);

	// Recreate albedo texture
	glGenTextures(1, &app->gBuffer.albedoTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.albedoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->gBuffer.albedoTexture, 0);

	// Recreate normal texture
	glGenTextures(1, &app->gBuffer.normalTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.normalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, app->gBuffer.normalTexture, 0);

	// Recreate position texture
	glGenTextures(1, &app->gBuffer.positionTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.positionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, app->gBuffer.positionTexture, 0);

	// Recreate depth texture
	glGenTextures(1, &app->gBuffer.depthTexture);
	glBindTexture(GL_TEXTURE_2D, app->gBuffer.depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->gBuffer.depthTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	// Check if the window has been resized
	if (app->displaySize.x != app->lastDisplaySize.x || app->displaySize.y != app->lastDisplaySize.y)
	{
		ResizeGBuffer(app);
		app->lastDisplaySize = app->displaySize;
	}

	CameraMovement(app);
}

void Render(App* app)
{
	switch (app->mode)
	{
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
		app->projectionMatrix = glm::perspective(glm::radians(app->camera.fov), aspectRatio, app->camera.zNear, app->camera.zFar);

		// Pass matrices to shader
		glUniformMatrix4fv(app->viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(app->viewMatrix));
		glUniformMatrix4fv(app->projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(app->projectionMatrix));

		for (const Entity& entity : app->entities)
		{
			glUniformMatrix4fv(app->modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(entity.transform.transformMatrix));

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
	case Mode_Deferred:
	{
		// GEOMETRY PASS
		glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer.fbo);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		// Use the geometry pass program
		Program& geometryPassProgram = app->programs[app->geometryPassProgramIdx];
		glUseProgram(geometryPassProgram.handle);

		glUniformMatrix4fv(app->geometryViewLoc, 1, GL_FALSE, glm::value_ptr(app->viewMatrix));
		glUniformMatrix4fv(app->geometryProjLoc, 1, GL_FALSE, glm::value_ptr(app->projectionMatrix));

		// Render all entities
		for (const Entity& entity : app->entities)
		{
			glUniformMatrix4fv(app->geometryModelLoc, 1, GL_FALSE, glm::value_ptr(entity.transform.transformMatrix));

			Model& entityModel = app->models[entity.modelIdx];
			Mesh& entityMesh = app->meshes[entityModel.meshIdx];

			for (u32 i = 0; i < entityMesh.submeshes.size(); ++i)
			{
				GLuint vao = FindVAO(entityMesh, i, geometryPassProgram);
				glBindVertexArray(vao);

				u32 submeshMaterialIdx = entityModel.materialIdx[i];
				Material& submeshMaterial = app->materials[submeshMaterialIdx];

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
				glUniform1i(app->geometryTexLoc, 0);

				Submesh& submesh = entityMesh.submeshes[i];
				glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
			}
		}

		// LIGHTING PASS

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		Program& lightingProgram = app->programs[app->gBuffer.deferredLightingProgramIdx];
		glUseProgram(lightingProgram.handle);

		glUniform1i(app->lightingBufferTypeLoc, app->renderpass_selected);

		// G-buffer textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, app->gBuffer.albedoTexture);
		glUniform1i(app->lightingAlbedoTexLoc, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, app->gBuffer.normalTexture);
		glUniform1i(app->lightingNormalTexLoc, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, app->gBuffer.positionTexture);
		glUniform1i(app->lightingPositionTexLoc, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, app->gBuffer.depthTexture);
		glUniform1i(app->lightingDepthTexLoc, 3);

		glUniform3fv(app->viewPosLoc, 1, glm::value_ptr(app->camera.position));

		// Directional lights
		glUniform1i(glGetUniformLocation(lightingProgram.handle, "uNumDirLights"), (int)app->dirLights.size());
		for (size_t i = 0; i < app->dirLights.size(); ++i)
		{
			std::string lightName = "uDirLights[" + std::to_string(i) + "]";
			glUniform3fv(glGetUniformLocation(lightingProgram.handle, (lightName + ".direction").c_str()), 1, glm::value_ptr(app->dirLights[i].direction));
			glUniform3fv(glGetUniformLocation(lightingProgram.handle, (lightName + ".color").c_str()), 1, glm::value_ptr(app->dirLights[i].color));
			glUniform1f(glGetUniformLocation(lightingProgram.handle, (lightName + ".intensity").c_str()), app->dirLights[i].intensity);
		}

		// Point lights
		glUniform1i(glGetUniformLocation(lightingProgram.handle, "uNumPointLights"), (int)app->pointLights.size());
		for (size_t i = 0; i < app->pointLights.size(); ++i)
		{
			std::string lightName = "uPointLights[" + std::to_string(i) + "]";
			glUniform3fv(glGetUniformLocation(lightingProgram.handle, (lightName + ".position").c_str()), 1, glm::value_ptr(app->pointLights[i].position));
			glUniform3fv(glGetUniformLocation(lightingProgram.handle, (lightName + ".color").c_str()), 1, glm::value_ptr(app->pointLights[i].color));
			glUniform1f(glGetUniformLocation(lightingProgram.handle, (lightName + ".intensity").c_str()), app->pointLights[i].intensity);
			glUniform1f(glGetUniformLocation(lightingProgram.handle, (lightName + ".radius").c_str()), app->pointLights[i].radius);
		}

		glBindVertexArray(app->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glUseProgram(0);

		// DEBUG VIEW(using textured mesh shader)
		/*if (app->renderpass_selected > 0)
		{
			glDisable(GL_DEPTH_TEST);
			glViewport(0, 0, app->displaySize.x, app->displaySize.y);

			Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
			glUseProgram(texturedMeshProgram.handle);

			glActiveTexture(GL_TEXTURE0);
			switch (app->renderpass_selected)
			{
			case 1: glBindTexture(GL_TEXTURE_2D, app->gBuffer.albedoTexture); break;
			case 2: glBindTexture(GL_TEXTURE_2D, app->gBuffer.normalTexture); break;
			case 3: glBindTexture(GL_TEXTURE_2D, app->gBuffer.positionTexture); break;
			case 4: glBindTexture(GL_TEXTURE_2D, app->gBuffer.depthTexture); break;
			}
			glUniform1i(app->texturedMeshProgram_uTexture, 0);

			glm::mat4 identity = glm::mat4(1.0f);
			glUniformMatrix4fv(app->modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(identity));
			glUniformMatrix4fv(app->viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(identity));
			glUniformMatrix4fv(app->projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(identity));

			glBindVertexArray(app->vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);
			glUseProgram(0);
		}*/
	}
	break;

	default:;
	}
}

void CleanUp(App* app)
{
	// Clean up textures
	for (auto& texture : app->textures) {
		glDeleteTextures(1, &texture.handle);
	}

	// Clean up meshes
	for (auto& mesh : app->meshes) {
		glDeleteBuffers(1, &mesh.vertexBufferHandle);
		glDeleteBuffers(1, &mesh.indexBufferHandle);

		for (auto& submesh : mesh.submeshes) {
			for (auto& vao : submesh.vaos) {
				glDeleteVertexArrays(1, &vao.handle);
			}
		}
	}

	// Clean up programs
	for (auto& program : app->programs) {
		glDeleteProgram(program.handle);
	}

	// Clean up GBuffer
	glDeleteFramebuffers(1, &app->gBuffer.fbo);
	glDeleteTextures(1, &app->gBuffer.albedoTexture);
	glDeleteTextures(1, &app->gBuffer.normalTexture);
	glDeleteTextures(1, &app->gBuffer.positionTexture);
	glDeleteTextures(1, &app->gBuffer.depthTexture);

	// Clean up quad
	glDeleteVertexArrays(1, &app->vao);
	glDeleteBuffers(1, &app->quadVBO);

	// Clean up light visualization resources
	glDeleteVertexArrays(1, &app->lightSphereVAO);
	glDeleteBuffers(1, &app->lightSphereVBO);
	glDeleteBuffers(1, &app->lightSphereEBO);
}

