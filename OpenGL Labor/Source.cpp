#define GLEW_STATIC
#include <gl\glew.h>

#define GLM_FORCE_RADIANS
#include "GLFW\glfw3.h"
#include "GL/glm/glm.hpp"
#include "GL/glm/gtc/matrix_transform.hpp"
#include "GL/glm/gtc/matrix_inverse.hpp"
#include "GL/glm/gtx/transform.hpp"
#include "GL/glm/gtc/type_ptr.hpp"
#include "GL/glm/gtc/random.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "assimp\importer.hpp"
#include "assimp\scene.h"
#include "assimp\postprocess.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     OPENGL INDEX ENUMERÁCIÓK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define UNIFORM_BUFFER_CAMERA 0
#define UNIFORM_BUFFER_LIGHT  1
#define UNIFORM_BUFFER_OBJECT 2

#define VERTEX_ATTRIB_POS    0
#define VERTEX_ATTRIB_UV     1
#define VERTEX_ATTRIB_NORMAL 2

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     ABLAK ÉS FRAMEBUFFER
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLFWwindow* s_window = NULL;
static GLint s_width = 640, s_height = 480;
static GLint s_bufferWidth = s_width, s_bufferHeight = s_height;
static GLuint s_textureDepth = 0, s_textureColor = 0;
static GLuint s_fbo = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     INPUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool s_run = true;

static GLdouble s_mouseX = -1.0, s_mouseY = -1.0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     KAMERA PARAMÉTEREK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLfloat s_camMovementSpeed = 20.0f;
static GLfloat s_camRotationSpeed = glm::radians(0.1f);

static GLfloat s_pitch = 0.0f;
static GLfloat s_yaw = 0.0f;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     UNIFORM PUFFEREK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLuint s_uboCamera, s_uboLight, s_uboObject;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     SHADEREK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLuint s_program = 0, s_programComposite = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     SKYBOX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLuint s_programSkybox = 0;
static GLuint s_textureSkybox = 0;

static std::string skyboxImages[] = 
{
	"right.jpg",
	"left.jpg",
	"top.jpg",
	"bottom.jpg",
	"back.jpg",
	"front.jpg" 
};

GLfloat dayNight = 1.0f;
glm::vec3 s_skyboxTintColor = glm::vec3(1.0f)*dayNight;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     KÖZÉPSÕ TÉGLALAP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SphereData
{
	glm::vec3 center;
	GLfloat radius;
	glm::mat4 s_sphereViewProjection;

	glm::vec3 velocity;
	GLfloat mass;
	int lockCollision;
};

static GLuint s_textureMiddle = 0;
std::vector<SphereData> s_sphereMovementData;
static GLuint s_sphereVboVertex = 0, s_sphereVboUV = 0, s_sphereVboNormal, s_sphereIbo = 0, s_sphereVao = 0;

std::vector<glm::vec3> s_spherePositions;
std::vector<glm::vec3> s_sphereNormals;
std::vector<glm::vec2> s_sphereUVs;
std::vector<GLuint> s_sphereIndices;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GLuint s_vboVertex = 0, s_vboUV = 0, s_vboNormal, s_ibo = 0, s_vao = 0;
static GLuint s_vboQuadVertex = 0, s_vboQuadUV = 0, s_iboQuad = 0, s_vaoQuad = 0;

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 tex;
	glm::vec3 normal;

	Vertex(glm::vec3 a, glm::vec2 b, glm::vec3 c)
	{
		pos = a;
		tex = b;
		normal = c;
	}

	Vertex()
	{

	}

};

struct Submesh 
{
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ibo;
	unsigned int m_indexCount;
	unsigned int m_materialIndex;
};

struct Material
{
	GLuint m_texture;
	GLfloat m_specular;
};

struct Mesh
{
	std::vector<Submesh> m_submeshes;
	std::vector<Material> m_materials;
};

static Mesh s_sponza;

struct CameraData
{
	glm::mat4 m_view;
	glm::mat4 m_projection;
	glm::vec4 m_eye = glm::vec4(0.0f, 0.0f, 2.0f, 0.0f);
	glm::vec4 m_right = glm::vec4(0.0f);
	glm::vec4 m_forward = glm::vec4(0.0f);
	glm::vec4 m_up = glm::vec4(0.0f);
} s_cameraData;

struct ObjectData
{
	glm::mat4 m_model;
	glm::mat4 m_normal;
}s_skyboxData, s_sponzaData, s_wallData, s_sphereData[3];

struct DirectionalLightData
{
	glm::vec4 m_ligthDir;
	glm::vec4 m_lightColor;
} s_directionalLight;

struct PointLightData
{
	glm::vec3 m_center;
	float _pad1;
	glm::vec3 m_diffuseColor;
	float _pad2;
	glm::vec3 m_specularColor;
	float _pad3;
	glm::vec3 m_ambientColor;
	GLfloat m_radius;
} s_pointLight;

static std::vector<glm::vec3> s_cubeVertices=
{
	//     6-------7
	//    /|      /|
	//   / |     / |
	//  4-------5  |
	//  |  2----|--3
	//  | /     | /
	//  |/      |/
	//  0-------1
	glm::vec3(-0.5f, -0.5f,  0.5f),
	glm::vec3( 0.5f, -0.5f,  0.5f),
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3( 0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f,  0.5f,  0.5f),
	glm::vec3( 0.5f,  0.5f,  0.5f),
	glm::vec3(-0.5f,  0.5f, -0.5f),
	glm::vec3( 0.5f,  0.5f, -0.5f),
};

static std::vector<glm::vec3> s_cubeNormals=
{
	glm::vec3(-0.33f, -0.33f,  0.33f),
	glm::vec3( 0.33f, -0.33f,  0.33f),
	glm::vec3(-0.33f, -0.33f, -0.33f),
	glm::vec3( 0.33f, -0.33f, -0.33f),

	glm::vec3(-0.33f,  0.33f,  0.33f),
	glm::vec3( 0.33f,  0.33f,  0.33f),
	glm::vec3(-0.33f,  0.33f, -0.33f),
	glm::vec3( 0.33f,  0.33f, -0.33f),
};

static std::vector<glm::vec2> s_cubeUVS=
{
	glm::vec2(0.0f, 0.0f),
	glm::vec2(1.0f, 0.0f),
	glm::vec2(0.0f, 1.0f),
	glm::vec2(1.0f, 1.0f),
};

static std::vector<GLuint> s_cubeIndices=
{
	//elsõ
	0, 1, 4, 5,
	//jobb
	1, 3, 5, 7,
	//hátsó
	3, 2, 7, 6,
	//bal
	2, 0, 6, 4,
	//alsó
	2, 3, 0, 1,
	//felsõ
	4, 5, 6, 7,
};

static std::vector<GLuint> s_faceIndices=
{
	0, 1, 2,
	2, 1, 3
};

static std::vector<glm::vec3> s_quadPositions = 
{
	glm::vec3( -1.0f, -1.0f, 0.0f), 
	glm::vec3(  1.0f, -1.0f, 0.0f), 
	glm::vec3(  1.0f,  1.0f, 0.0f), 
	glm::vec3( -1.0f,  1.0f, 0.0f), 
};

static std::vector<glm::vec2> s_quadUVS = 
{
	glm::vec2( 0.0f,  0.0f), 
	glm::vec2( 1.0f,  0.0f), 
	glm::vec2( 1.0f,  1.0f), 
	glm::vec2( 0.0f,  1.0f), 
};

static std::vector<GLuint> s_quadIndices = 
{
	0, 1, 2,
	0, 2, 3,
};

std::vector<glm::vec3> s_positions;
std::vector<glm::vec3> s_normals;
std::vector<glm::vec2> s_uvs;
std::vector<GLuint> s_indices;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     SHADER BETÖLTÉS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string loadTextFile(const std::string& fileName)
{
    std::string result;

    std::ifstream fstream(fileName);
    if (fstream.good())
    {
        std::stringstream inputStream;
        inputStream << fstream.rdbuf();

        result=inputStream.str();
    }
    
    return result;
}

static GLchar s_buffer[4098];

GLuint loadShader(const std::string& fileName, GLenum type)
{
	std::string source = loadTextFile(fileName);

	if (source.empty())
		return 0;

	GLuint shader = glCreateShader(type);

	const GLchar* sourcePtr[] = { source.c_str() };

	glShaderSource(shader, 1, sourcePtr, NULL);

	glCompileShader(shader);

	GLint status;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		glGetShaderInfoLog(shader, sizeof(s_buffer) / sizeof(GLchar), NULL, s_buffer);
		std::cout << s_buffer << std::endl;
	}

	return shader;
}

GLuint loadProgram(const std::string& baseName)
{
	/** Töltsük be a shadereket. */
	GLuint vs = loadShader(baseName + ".vs", GL_VERTEX_SHADER);
	GLuint gs = loadShader(baseName + ".gs", GL_GEOMETRY_SHADER);
	GLuint fs = loadShader(baseName + ".fs", GL_FRAGMENT_SHADER);

	/** Ezek után hozzuk létre az õket összefogó shader programot. */
	GLuint program = glCreateProgram();

	/** Csatoljuk a shadereket. */
	if (vs != 0) glAttachShader(program, vs);
	if (gs != 0) glAttachShader(program, gs);
	if (fs != 0) glAttachShader(program, fs);

	/** Végezetül linkeljük a shader programunkat, hogy használni tudjuk. */
	glLinkProgram(program);

	GLint status;

	/** Lekérjük a linkelés eredményét. */
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	/** Ha a linkelés sikertelen, írjuk ki a hiba okát. */
	if (status == GL_FALSE)
	{
		glGetProgramInfoLog(program, sizeof(s_buffer) / sizeof(GLchar), NULL, s_buffer);
		std::cout << s_buffer << std::endl;
	}

	/** Válasszuk le a shadereket a programról. Ha ezt nem tesszük, a következõ
	hívások során nem fogja õket törölni a rendszer. */
	if (vs != 0) glDetachShader(program, vs);
	if (gs != 0) glDetachShader(program, gs);
	if (fs != 0) glDetachShader(program, fs);

	/** Most már törölhetjük a shader objektumokat. A program linkelése után
	nincs szükségünk rájuk. */
	if (vs != 0) glDeleteShader(vs);
	if (gs != 0) glDeleteShader(gs);
	if (fs != 0) glDeleteShader(fs);

	/** Visszaadjuk a kész programot. */
	return program;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     TEXTÚRA BETÖLTÉS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool loadTexture(GLuint& texture, const std::string& fileName)
{
	int x, y, n;
	auto image = stbi_load(fileName.c_str(), &x, &y, &n, 4);

	if (image == NULL)
	{
		std::cout << "Error loading texture: " << fileName << std::endl;
		return false;
	}

	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);

	return true;
}

bool loadCubeMap(GLuint& texture, std::string* skyboxImages)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	for (int i = 0; i < 6; ++i)
	{
		auto fileName = "Skybox\\" + skyboxImages[i];

		int x, y, n;
		auto image = stbi_load(fileName.c_str(), &x, &y, &n, 4);

		if (image == NULL)
		{
			std::cout << "Error loading texture: " << fileName << std::endl;
			return false;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

		stbi_image_free(image);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     MESH BETÖLTÉS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void uploadVertexData(GLuint& vbo, GLuint& ibo, GLuint& vao, const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
{
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(VERTEX_ATTRIB_POS);
	glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL);
	glEnableVertexAttribArray(VERTEX_ATTRIB_UV);
	glVertexAttribPointer(VERTEX_ATTRIB_POS, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)sizeof(glm::vec3));
	glVertexAttribPointer(VERTEX_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(sizeof(glm::vec3)+sizeof(glm::vec2)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBindVertexArray(0);
}

void loadMeshSubmeshes(Mesh& mesh, unsigned int index, const aiMesh* paiMesh)
{
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	vertices.reserve(paiMesh->mNumVertices);
	indices.reserve(paiMesh->mNumFaces*3);

	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) 
	{
		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

		Vertex vertex(
			glm::vec3(pPos->x, pPos->y, pPos->z),
			glm::vec2(pTexCoord->x, pTexCoord->y),
			glm::vec3(pNormal->x, pNormal->y, pNormal->z));

		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) 
	{
		const aiFace& face = paiMesh->mFaces[i];

		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	Submesh& submesh = mesh.m_submeshes[index];
	
	submesh.m_materialIndex = paiMesh->mMaterialIndex;
	submesh.m_indexCount = paiMesh->mNumFaces * 3;

	uploadVertexData(submesh.m_vbo, submesh.m_ibo, submesh.m_vao, vertices, indices);
}

bool loadMeshMaterials(Mesh& mesh, const aiScene* pScene, const std::string& dir)
{
	bool ret = true;

	for (unsigned int i = 0; i < pScene->mNumMaterials; i++)
	{
		const aiMaterial* pMaterial = pScene->mMaterials[i];

		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString path;

			mesh.m_materials[i].m_texture = 0;

			pMaterial->Get(AI_MATKEY_SHININESS, mesh.m_materials[i].m_specular);

			if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
			{
				std::string fullPath = dir + "\\" + path.data;

				if (!loadTexture(mesh.m_materials[i].m_texture, fullPath))
				{
					printf("Error loading texture '%s'\n", fullPath.c_str());
					ret = false;
				}
			}
		}

		if (mesh.m_materials[i].m_texture == 0)
		{
			ret = loadTexture(mesh.m_materials[i].m_texture, "white-512.jpg");
		}
	}

	return ret;
}

bool loadMesh(Mesh& mesh, const std::string& dir, const std::string& fileName)
{
	bool ret = false;
	Assimp::Importer importer;

	std::string fullPath = dir + "\\" + fileName;

	const aiScene* pScene = importer.ReadFile(fullPath.c_str(), aiProcess_Triangulate
		| aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

	if (pScene == NULL)
	{
		printf("Error parsing '%s': '%s'\n", fullPath.c_str(), importer.GetErrorString());
		return false;
	}

	mesh.m_submeshes.resize(pScene->mNumMeshes);
	mesh.m_materials.resize(pScene->mNumMaterials);

	// Initialize the meshes in the scene one by one
	for (unsigned int i = 0; i < mesh.m_submeshes.size(); i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		loadMeshSubmeshes(mesh, i, paiMesh);
	}

	return loadMeshMaterials(mesh, pScene, dir);
}

void unloadMesh(Mesh& mesh)
{
	for (auto& submesh : mesh.m_submeshes)
	{
		glDeleteBuffers(1, &submesh.m_vbo);
		glDeleteBuffers(1, &submesh.m_ibo);
		glDeleteVertexArrays(1, &submesh.m_vao);
	}

	for (auto& material : mesh.m_materials)
	{
		glDeleteTextures(1, &material.m_texture);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::pair<glm::vec3, glm::vec3> getOrthogonalComponents(glm::vec3 a, glm::vec3 v)
{
	glm::vec3 m = (glm::dot(a, v) / glm::dot(v, v))*v;
	glm::vec3 u = a - m;

	return std::make_pair(m, u);
}

glm::vec3 mirrorVector(const glm::vec3 a, const glm::vec3 v)
{
	const auto& orthoComponents = getOrthogonalComponents(a, v);
	return 2.0f * orthoComponents.first - a;
}

bool checkSphereSphereCollision(const SphereData& sd1, const SphereData& sd2)
{
	if (glm::distance(sd1.center, sd2.center) > sd1.radius + sd2.radius)
		return false;

	glm::vec3 w = sd2.center - sd1.center;

	GLfloat angle = glm::dot(w, sd1.velocity) / (glm::length(sd1.velocity)*glm::length(w));
	return angle > 0;
}

std::vector<glm::vec3> wallNormals =
{
	//elsõ
	glm::vec3(0, 0, 1),
	//jobb
	glm::vec3(1, 0, 0),
	//hátsó
	glm::vec3(0, 0, -1),
	//bal
	glm::vec3(-1, 0, 0),
	//alsó
	glm::vec3(0, -1, 0),
	//felsõ
	glm::vec3(0, 1, 0),
};

glm::vec3 getWallNormal(int index)
{
	return wallNormals[index];
}

std::vector<glm::vec3> wallPoints =
{
	//elsõ
	glm::vec3(0.5, 0.5, 0.5),
	//jobb
	glm::vec3(0.5, 0.5, 0.5),
	//hátsó
	glm::vec3(0.5, 0.5, -0.5),
	//bal
	glm::vec3(-0.5, 0.5, 0.5),
	//alsó
	glm::vec3(0.5, -0.5, 0.5),
	//felsõ
	glm::vec3(0.5, 0.5, 0.5),
};

glm::vec3 gelWallPoint(int index)
{
	return glm::vec3(s_wallData.m_model * glm::vec4(wallPoints[index], 1));
}

bool checkSphereWallCollision(const SphereData& sd, int index)
{
	glm::vec3 wallNormal = getWallNormal(index);
	glm::vec3 sphereCenter = sd.center;
	glm::vec3 wallPoint = gelWallPoint(index);

	GLfloat Ax = wallNormal.x * sphereCenter.x;
	GLfloat By = wallNormal.y * sphereCenter.y;
	GLfloat Cz = wallNormal.z * sphereCenter.z;
	GLfloat	D = -wallNormal.x*wallPoint.x - wallNormal.y*wallPoint.y - wallNormal.z*wallPoint.z;

	if (glm::abs(Ax + By + Cz + D) > sd.radius)
		return false;

	GLfloat angle = glm::dot(getWallNormal(index), sd.velocity) / glm::length(sd.velocity);
	return angle > 0;
}

glm::vec3 resolveWallCollision(const SphereData& sd, int index)
{
	return mirrorVector(-sd.velocity, getWallNormal(index));
}

std::pair<glm::vec3, glm::vec3> resolveCollision(const SphereData& sd1, const SphereData& sd2)
{
	glm::vec3 w = sd2.center - sd1.center;
	GLfloat r = sd1.mass / sd2.mass;

	glm::vec3 s1 = sd1.velocity;
	glm::vec3 s2 = sd2.velocity;
	glm::vec3 m1 = s1 - s2;
	const auto& orthoComponent = getOrthogonalComponents(m1, w);

	glm::vec3 m1_vesszo = orthoComponent.first;
	glm::vec3 u1 = orthoComponent.second;

	glm::vec3 m2 = ((r - 1.0f) / (r + 1.0f))*m1_vesszo + u1;
	glm::vec3 a2 = (2 * r*m1_vesszo) / (r + 1);

	return std::make_pair(m2 + s2, a2 + s2);
}

GLfloat getSphereRadius()
{
	return glm::linearRand(0.4f, 0.7f);
}

glm::vec3 getSphereVelocity()
{
	return 12.0f * glm::vec3(
		glm::linearRand(-0.5f, 0.5f),
		glm::linearRand(-0.5f, 0.5f),
		glm::linearRand(-0.5f, 0.5f));
}

GLfloat getSphereMass()
{
	return glm::linearRand(8.0, 12.0);
}

void initSphereCenters()
{
	SphereData sphere;

	sphere.center = glm::vec3(0.0f, 5.0f, 0.0f);
	sphere.radius = getSphereRadius();
	sphere.velocity = glm::vec3(-5.0f, -5.0f, -5.0f);
	sphere.mass = getSphereMass();
	s_sphereMovementData.push_back(sphere);

	sphere.center = glm::vec3(5.0f, 5.0f, 5.0f);
	sphere.radius = getSphereRadius();
	sphere.velocity = glm::vec3(-5.0f, -5.0f, -5.0f);
	sphere.mass = getSphereMass();
	s_sphereMovementData.push_back(sphere);

	sphere.center = glm::vec3(7.0f, 5.0f, 5.0f);
	sphere.radius = getSphereRadius();
	sphere.velocity = glm::vec3(-5.0f, -5.0f, -5.0f);
	sphere.mass = getSphereMass();
	s_sphereMovementData.push_back(sphere);
}

glm::vec3 GenerateVertex(const float& phi, const float& theta)
{
	return glm::vec3(
		cos(theta)*sin(phi),
		cos(phi),
		-sin(theta)*sin(phi)
		);
}

void GenSphereCoordinates(unsigned detail)
{
	float step = glm::pi<float>() / detail;

	for (int i = 0; i <= detail; i++)
	{
		float phi = i*step;

		for (int j = 0; j <= detail * 2; j++)
		{
			float theta = j*step;

			s_spherePositions.push_back(GenerateVertex(phi, theta));
			s_sphereNormals.push_back(glm::normalize(GenerateVertex(phi, theta)));
			s_sphereUVs.push_back(glm::vec2(j * (1.0 / (detail * 2)), 1.0 - i * (1.0 / detail)));
		}
	}
}

GLuint genIndex(unsigned detail, int i, int j)
{
	return i * (detail * 2 + 1) + j;
}

void genSphereIndices(unsigned detail)
{
	for (int i = 0; i < detail; i++)
	{
		for (int j = 0; j<detail * 2; j++)
		{
			s_sphereIndices.push_back(genIndex(detail, i, j));
			s_sphereIndices.push_back(genIndex(detail, i + 1, j));
			s_sphereIndices.push_back(genIndex(detail, i + 1, j + 1));

			s_sphereIndices.push_back(genIndex(detail, i, j));
			s_sphereIndices.push_back(genIndex(detail, i + 1, j + 1));
			s_sphereIndices.push_back(genIndex(detail, i, j + 1));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     JELENET INICIALIZÁCIÓ
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initLightSources()
{
    /** Irányított fényforrás iránya és színe. */
	s_directionalLight.m_ligthDir = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
	s_directionalLight.m_lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    
    /** Pontszerû fényforrás helye, sugara, színe és csillapodásai. */
	s_pointLight.m_center = glm::vec3(0.0f, 1000.0f, 0.0f);
	s_pointLight.m_radius = 1000.0f;
	s_pointLight.m_ambientColor = glm::vec3(1,1,1)*0.15f;
	s_pointLight.m_diffuseColor = glm::vec3(1, 1, 1);
	s_pointLight.m_specularColor = glm::vec3(1, 1, 1);
}

void initMatrices()
{
    /** Az eredeti elõre és jobbra mutató vektorok. */
	static glm::vec4 FORWARD(0.0f, 0.0f, -1.0f, 1.0f);
	static glm::vec4 RIGHT(1.0f, 0.0f, 0.0f, 1.0f);
	static glm::vec4 UP(0.0f, 1.0f, 0.0f, 1.0f);
	
    /** Konstruáljunk meg egy forgatási mátrixot a pitch és yaw értékek alapján */
	glm::mat4 rotation = glm::rotate(s_yaw, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(s_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	
    /** Számítsuk ki a három tengelyhez tartozó vektorokat. */
	s_cameraData.m_forward = rotation * FORWARD;
	s_cameraData.m_right = rotation * RIGHT;
	s_cameraData.m_up = rotation * UP;

    /** Konstruáljuk meg az MVP (model-view-projection) mátrixokat. */
	s_cameraData.m_view = glm::lookAt(glm::vec3(s_cameraData.m_eye), glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward), glm::vec3(0.0f, 1.0f, 0.0f));
	s_cameraData.m_projection = glm::perspective(glm::radians(65.0f), GLfloat(s_width)/GLfloat(s_height), 0.1f, 10000.0f);

	s_skyboxData.m_model = glm::scale(glm::vec3(5000.0f));
	s_skyboxData.m_normal = glm::inverseTranspose(s_skyboxData.m_model);

	s_sponzaData.m_model = glm::scale(glm::vec3(2.0f));
	s_sponzaData.m_normal = glm::inverseTranspose(s_sponzaData.m_model);

	s_wallData.m_model = glm::translate(glm::vec3(0,50,0)) * glm::scale(glm::vec3(50.0f));
	s_wallData.m_normal = glm::inverseTranspose(s_wallData.m_model);

	s_sphereData[0].m_model = glm::translate(glm::vec3(0,50,0)) * glm::scale(glm::vec3(100.0f));
	s_sphereData[0].m_normal = glm::inverseTranspose(s_sphereData[0].m_model);

	s_sphereData[1].m_model = glm::translate(glm::vec3(0, 30, 0)) * glm::scale(glm::vec3(100.0f));
	s_sphereData[1].m_normal = glm::inverseTranspose(s_sphereData[1].m_model);

	s_sphereData[2].m_model = glm::translate(glm::vec3(0, 10, 0)) * glm::scale(glm::vec3(100.0f));
	s_sphereData[2].m_normal = glm::inverseTranspose(s_sphereData[2].m_model);
}

void initGeometry()
{
	/** Töltsük fel a kockához tartozó vektorokat. */
	for (size_t i=0;i<s_cubeIndices.size(); ++i)
	{
		/** A pozíció és normálvektor meghatározásához használjuk az elõre
		    letárolt értékeket. */
		s_positions.push_back(s_cubeVertices[s_cubeIndices[i]]);
		s_normals.push_back(s_cubeNormals[s_cubeIndices[i]]);

		/** Az UV koordinátákhoz azt a tényt használjuk ki, hogy minden 4 index
		    tesz ki egy lapot, és a lapok bejárási iránya megegyezik. */
		s_uvs.push_back(s_cubeUVS[i%4]);
	}

	/** Generáljuk ki a 6 laphoz tartozó indexeket. */
	for (int i=0;i<6;i++)
		for (size_t j=0;j<s_faceIndices.size();++j)
			s_indices.push_back(s_faceIndices[j] + i*4);

	GenSphereCoordinates(20);
	genSphereIndices(20);
}

void initBuffers()
{
	/** Hozzuk létre a uniform puffereket. */
	glGenBuffers(1, &s_uboLight);
	glGenBuffers(1, &s_uboCamera);
	glGenBuffers(1, &s_uboObject);
	
	glGenBuffers(1, &s_sphereVboVertex);
	glGenBuffers(1, &s_sphereVboUV);
	glGenBuffers(1, &s_sphereVboNormal);
	glGenBuffers(1, &s_sphereIbo);
	glGenVertexArrays(1, &s_sphereVao);

	/** Hozzuk létre a szükséges VAO és VBO-kat. */
	glGenBuffers(1, &s_vboVertex);
	glGenBuffers(1, &s_vboUV);
	glGenBuffers(1, &s_vboNormal);
	glGenBuffers(1, &s_ibo);
	glGenVertexArrays(1, &s_vao);
    
	glBindBuffer(GL_ARRAY_BUFFER, s_vboVertex);
	glBufferData(GL_ARRAY_BUFFER, s_positions.size() * sizeof(glm::vec3), s_positions.data(), GL_STATIC_DRAW);
    /** Lecsatoljuk a puffert. */
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, s_vboUV);
	glBufferData(GL_ARRAY_BUFFER, s_uvs.size() * sizeof(glm::vec2), s_uvs.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
	glBindBuffer(GL_ARRAY_BUFFER, s_vboNormal);
	glBufferData(GL_ARRAY_BUFFER, s_normals.size() * sizeof(glm::vec3), s_normals.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_indices.size() * sizeof(GLuint), s_indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//Sphere buffers
	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboVertex);
	glBufferData(GL_ARRAY_BUFFER, s_spherePositions.size() * sizeof(glm::vec3), s_spherePositions.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboUV);
	glBufferData(GL_ARRAY_BUFFER, s_sphereUVs.size() * sizeof(glm::vec2), s_sphereUVs.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboNormal);
	glBufferData(GL_ARRAY_BUFFER, s_sphereNormals.size() * sizeof(glm::vec3), s_sphereNormals.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Sphere indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_sphereIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_sphereIndices.size() * sizeof(GLuint), s_sphereIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(s_vao);
	glEnableVertexAttribArray(VERTEX_ATTRIB_POS);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboVertex);
	glVertexAttribPointer(VERTEX_ATTRIB_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(VERTEX_ATTRIB_UV);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboUV);
	glVertexAttribPointer(VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboNormal);
	glVertexAttribPointer(VERTEX_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);

	glBindVertexArray(0);

	//Sphere VAO
	glBindVertexArray(s_sphereVao);
	glEnableVertexAttribArray(VERTEX_ATTRIB_POS);
	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboVertex);
	glVertexAttribPointer(VERTEX_ATTRIB_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(VERTEX_ATTRIB_UV);
	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboUV);
	glVertexAttribPointer(VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL);
	glBindBuffer(GL_ARRAY_BUFFER, s_sphereVboNormal);
	glVertexAttribPointer(VERTEX_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_sphereIbo);
	glBindVertexArray(0);

	/** Hozzuk létre a négyzethez tartozó puffereket.. */
	glGenBuffers(1, &s_vboQuadVertex);
	glGenBuffers(1, &s_vboQuadUV);
	glGenBuffers(1, &s_iboQuad);
	glGenVertexArrays(1, &s_vaoQuad);
    
	/** Töltsük fel a négyzet puffereit. */
	glBindBuffer(GL_ARRAY_BUFFER, s_vboQuadVertex);
    glBufferData(GL_ARRAY_BUFFER, s_quadPositions.size() * sizeof(glm::vec3), s_quadPositions.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, s_vboQuadUV);
	glBufferData(GL_ARRAY_BUFFER, s_quadUVS.size() * sizeof(glm::vec2), s_quadUVS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_iboQuad);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_quadIndices.size() * sizeof(GLuint), s_quadIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/** Konfiguráljuk a négyzethez tartozó VAO-t. */
    glBindVertexArray(s_vaoQuad);
    glEnableVertexAttribArray(VERTEX_ATTRIB_POS);
    glBindBuffer(GL_ARRAY_BUFFER, s_vboQuadVertex);
    glVertexAttribPointer(VERTEX_ATTRIB_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(VERTEX_ATTRIB_UV);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboQuadUV);
	glVertexAttribPointer(VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_iboQuad);
    glBindVertexArray(0);
}

void initFrameBuffer()
{
	glGenTextures(1, &s_textureDepth);
	glBindTexture(GL_TEXTURE_2D, s_textureDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, s_bufferWidth, s_bufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);

    /** Hozzuk létre a framebuffer szín textúráját. */
	glGenTextures(1, &s_textureColor);
	glBindTexture(GL_TEXTURE_2D, s_textureColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_bufferWidth, s_bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

    /** Hozzuk létre a framebuffer objektumot. */
	glGenFramebuffers(1, &s_fbo);
    
    /** Mint minden eddigi objektumot, ezt is csatolnunk kell, hogy módosítani tudjuk. */
	glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    
    /** A 0. indexû színpuffert állítsuk a szín textúránkra. */
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_textureColor, 0);
    
    /** A mélységpuffert állítsuk a mélység textúránkra. */
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s_textureDepth, 0);
    
    /** Kérjük le a framebuffer állapotát. */
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    /** Ha az állapot nem "Complete", akkor valamit rosszul csináltunk. */
	if (status != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "framebuffer not complete, cause: " << status << std::endl;

    /** Válasszuk le a framebuffert. */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initShaders()
{
	s_program = loadProgram("first");
	s_programComposite = loadProgram("composite");
	s_programSkybox = loadProgram("skybox");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     JELENET TÖRLÉS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cleanUpScene()
{
	unloadMesh(s_sponza);

    /** Töröljük a shader programot. */
    glDeleteProgram(s_program);
    glDeleteProgram(s_programComposite);
	glDeleteProgram(s_programSkybox);

    /** Töröljük a framebuffert és a hozzá tarotó textúrákat. */
	glDeleteFramebuffers(1, &s_fbo);
	glDeleteTextures(1, &s_textureColor);
	glDeleteTextures(1, &s_textureDepth);
	glDeleteTextures(1, &s_textureSkybox);

    /** Töröljük a VAO és VBO-kat. */
	glDeleteVertexArrays(1, &s_vao);
	glDeleteBuffers(1, &s_ibo);
	glDeleteBuffers(1, &s_vboVertex);
	glDeleteBuffers(1, &s_vboNormal);
	glDeleteBuffers(1, &s_vboUV);

	glDeleteVertexArrays(1, &s_vaoQuad);
	glDeleteBuffers(1, &s_iboQuad);
	glDeleteBuffers(1, &s_vboQuadVertex);
	glDeleteBuffers(1, &s_vboQuadUV);
    
    /** Töröljük a uniform puffereket. */
	glDeleteBuffers(1, &s_uboLight);
	glDeleteBuffers(1, &s_uboCamera);
	glDeleteBuffers(1, &s_uboObject);

	glDeleteVertexArrays(1, &s_sphereVao);
	glDeleteBuffers(1, &s_sphereIbo);
	glDeleteBuffers(1, &s_sphereVboVertex);
	glDeleteBuffers(1, &s_sphereVboUV);
	glDeleteBuffers(1, &s_sphereVboNormal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     JELENET KIRAJZOLÁSA
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void uploadCameraUniforms()
{
	/** Töltsük fel a kamera uniform pufferét. */
	glBindBuffer(GL_UNIFORM_BUFFER, s_uboCamera);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &s_cameraData, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
    /** Csatoljuk a puffereket a megfelelõ UBO indexekhez. */
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_CAMERA, s_uboCamera);
}

void uploadLightUniforms()
{
	/** Töltsük fel a fényforrás. */
	glBindBuffer(GL_UNIFORM_BUFFER, s_uboLight);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightData), &s_pointLight, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_LIGHT, s_uboLight);
}

void uploadObjectUniforms(const ObjectData& data)
{
	/** Töltsük fel a fényforrás. */
	glBindBuffer(GL_UNIFORM_BUFFER, s_uboObject);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectData), &data, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_OBJECT, s_uboObject);
}

void initScene()
{
	initSphereCenters();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	loadCubeMap(s_textureSkybox, skyboxImages);

	loadMesh(s_sponza, "Sponza", "sponza.obj");

	loadTexture(s_textureMiddle, "bricks.jpg");

	initLightSources();

	initMatrices();

	initGeometry();

	initBuffers();

	initFrameBuffer();

	initShaders();
}

void renderSkybox()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	uploadObjectUniforms(s_skyboxData);

	glUseProgram(s_programSkybox);

	glUniform3f(0, s_skyboxTintColor.x, s_skyboxTintColor.y, s_skyboxTintColor.z);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, s_textureSkybox);

	glBindVertexArray(s_vao);
	glDrawElements(GL_TRIANGLES, s_indices.size(), GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}

void renderObjects()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glDepthMask(GL_TRUE);
	
	uploadObjectUniforms(s_sponzaData);

	glUseProgram(s_program);

	for (const auto& submesh : s_sponza.m_submeshes)
	{
		const auto& material = s_sponza.m_materials[submesh.m_materialIndex];

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, material.m_texture);

		glBindVertexArray(submesh.m_vao);
		glDrawElements(GL_TRIANGLES, submesh.m_indexCount, GL_UNSIGNED_INT, NULL);
	}

	glBindVertexArray(0);
}

void renderMidLightsources()
{
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);*/

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);

	glDepthMask(GL_TRUE);

	glUseProgram(s_program);

	glBindVertexArray(s_sphereVao);
	for (const auto& sphere : s_sphereData)
	{
		uploadObjectUniforms(sphere);

		//Draw the sphere
		glDrawElements(GL_TRIANGLES, s_sphereIndices.size(), GL_UNSIGNED_INT, NULL);
	}
	glBindVertexArray(0);

	uploadObjectUniforms(s_wallData);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_textureMiddle);

	glBindVertexArray(s_vao);
	glDrawElements(GL_TRIANGLES, s_indices.size(), GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}

void renderBasePass()
{    
	/** Csatoljuk a framebuffer objektumot és beállítjuk a viewportot a teljes bufferre. */
	glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
	glViewport(0, 0, s_bufferWidth, s_bufferHeight);

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	uploadCameraUniforms();
	uploadLightUniforms();

	renderSkybox();

	renderObjects();

	renderMidLightsources();
}

void renderCompositePass()
{
	/** Csatoljuk a default framebuffert és állítsuk be a viewportot. */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, s_width, s_height);

	glDisable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	
	/** Tisztítsuk ki. */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/** Csatoljuk a programot. */
    glUseProgram(s_programComposite);
	
	/** Csatoljuk a jelenetet tartalmazó textúrát. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_textureColor);

	/** Rajzoljuk ki az ablakot lefedõ négyzetet. */
    glBindVertexArray(s_vaoQuad);
	glDrawElements(GL_TRIANGLES, s_quadIndices.size(), GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);

	/** Válasszuk le a textúrát, hogy ismét tudjunk rajzolni bele. */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void renderScene()
{
    renderBasePass();
	renderCompositePass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     INPUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void keyPressed(GLFWwindow* window, GLint key, GLint scanCode, GLint action, GLint mods)
{
	/** Billentyû leütés. */
	if (action==GLFW_PRESS || action==GLFW_REPEAT)
	{
		switch(key)
		{
		/** Az elõre mozgáshoz a forward vektort tudjuk felhasználni. */
		case GLFW_KEY_W:
			s_cameraData.m_eye += s_cameraData.m_forward * s_camMovementSpeed;
			s_pointLight.m_center = glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward*1000.0f);
			break;
		
		/** Hátra a forward vektor negáltját használjuk. */
		case GLFW_KEY_S: s_cameraData.m_eye -= s_cameraData.m_forward * s_camMovementSpeed;
		s_pointLight.m_center = glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward*1000.0f);
		break;
		
		/** Balra a right vektor negáltját... */
		case GLFW_KEY_A: s_cameraData.m_eye -= s_cameraData.m_right * s_camMovementSpeed;
		s_pointLight.m_center = glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward*1000.0f);
		break;
		
		/** ...jobbra pedig magát a right vektort tudjuk felhasználni. */
		case GLFW_KEY_D: s_cameraData.m_eye += s_cameraData.m_right * s_camMovementSpeed;
		s_pointLight.m_center = glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward*1000.0f);
		break;

		case GLFW_KEY_ENTER:
			std::cout << s_cameraData.m_eye.x << ", " << s_cameraData.m_eye.y << ", " << s_cameraData.m_eye.z << std::endl;
			break;

		case GLFW_KEY_N: 
			if (dayNight >= 0.0f)
			{
				dayNight-=0.03f;
				s_skyboxTintColor = glm::vec3(1.0f)*dayNight;
			}
			break;

		case GLFW_KEY_M: 
			if (dayNight <= 1.0f)
			{
				dayNight += 0.03f;
				s_skyboxTintColor = glm::vec3(1.0f)*dayNight;
			}
			
			break;
		
		/** Kilépés escape-re. */
		case GLFW_KEY_ESCAPE:
			s_run = false;
			break;
		}
    }
	
	/** Ha történt módosítás, ha nem, állítsuk elõ újra az MVP mátrixokat. */
	initMatrices();
	
	/** Erre pedig a GLFW-nek van szüksége. */
	glfwPollEvents();
}

void mouseMoved(GLFWwindow* window, GLdouble x, GLdouble y)
{
	if (s_mouseX == -1.0)
	{
		s_mouseX = x;
		s_mouseY = y;
		return;
	}
	
	/** Számoljuk ki a mozgás mértékét. */
	GLdouble deltaX = x - s_mouseX;
	GLdouble deltaY = y - s_mouseY;
	
	/** Tároljuk el az új helyét. */
	s_mouseX = x;
	s_mouseY = y;
	
	/** Számítsuk ki az új pitch értéket. Az értéket korlátozzuk [-89°, 89°] tartományba. Ehhez
	    a 'clamp' nevû függvényt tudjuk használni. */
	s_pitch = glm::clamp<GLfloat>(s_pitch - deltaY * s_camRotationSpeed, glm::radians(-89.0f), glm::radians(179.0f));

	/** A kamera forgatási sebessége. */
	s_yaw = s_yaw - deltaX * s_camRotationSpeed;
	
	/** Ha történt módosítás, ha nem, állítsuk elõ újra az MVP mátrixokat. */
	initMatrices();

	s_pointLight.m_center = glm::vec3(s_cameraData.m_eye + s_cameraData.m_forward*1000.0f);
	
	/** Erre pedig a GLFW-nek van szüksége. */
	glfwPollEvents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                     MAIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc,char** argv)
{
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	s_window = glfwCreateWindow(s_width, s_height, "Hello OpenGL", NULL, NULL);
	if (s_window==NULL)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(s_window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(s_window);

	initScene();

	glfwSetKeyCallback(s_window, keyPressed);
	glfwSetCursorPosCallback(s_window, mouseMoved);
	glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	while (s_run && !glfwWindowShouldClose(s_window))
	{
		glfwPollEvents();
        
		renderScene();
        
		glfwSwapBuffers(s_window);
	}

	cleanUpScene();

	glfwTerminate();

	return 0;
}