#version 430

/** A uniform puffer indexek. */
#define UNIFORM_BUFFER_CAMERA 0
#define UNIFORM_BUFFER_LIGHT  1
#define UNIFORM_BUFFER_OBJECT 2

/** Attribútum indexek. */
#define VERTEX_ATTRIB_POS    0
#define VERTEX_ATTRIB_UV     1
#define VERTEX_ATTRIB_NORMAL 2

/** Kamera uniform pufferei. */
layout (std140, binding = UNIFORM_BUFFER_CAMERA) uniform CameraData
{
    mat4 mView;
    mat4 mProjection;
    vec3 vEye;
    vec3 vRight;
    vec3 vForward;
    vec3 vUp;
} sCameraData;

layout (std140, binding = UNIFORM_BUFFER_OBJECT) uniform ObjectData
{
    mat4 mModel;
    mat4 mNormal;
} sObjectData;

/** Input attribútumok. */
layout (location = VERTEX_ATTRIB_POS) in vec3 vPos;

/** Output attribútumok. */
out vec3 vPosVS;

void main()
{    
    /** Adjuk tovább a világkoordinátabeli koordinátákat. */
    vPosVS = vec3(sObjectData.mModel * vec4(vPos, 1));
    
	vec3 vPosCS = vec3(sCameraData.mView * vec4(vPosVS, 0));
	
    /** Számoljuk ki a vertex pozícióját. */
    gl_Position = sCameraData.mProjection * vec4(vPosCS, 1);
}