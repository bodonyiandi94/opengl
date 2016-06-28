#version 430

/** A uniform puffer indexek. */
#define UNIFORM_BUFFER_CAMERA 0
#define UNIFORM_BUFFER_LIGHT  1

/** Attribútum indexek. */
#define VERTEX_ATTRIB_POS    0
#define VERTEX_ATTRIB_UV     1
#define VERTEX_ATTRIB_NORMAL 2

/** Kamera uniform pufferei. */
layout (std140, binding = UNIFORM_BUFFER_CAMERA) uniform CameraData
{
    mat4 mModel;
    mat4 mView;
    mat4 mProjection;
    mat4 mNormal;
    vec3 vEye;
    vec3 vRight;
    vec3 vForward;
    vec3 vUp;
} sCameraData;

/** Input attribútumok. */
layout (location = VERTEX_ATTRIB_POS) in vec3 vPos;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUV;
layout (location = VERTEX_ATTRIB_NORMAL) in vec3 vNormal;

/** Output attribútumok. */
out vec2 vUVVS;
out vec3 vNormalVS;
out vec3 vPosVS;

void main()
{
    /** Adjuk tovább az UV koordinátát. */
    vUVVS = vUV;
    
    /** Számoljuk ki a transzformált normálvektort. */
    vNormalVS = normalize(vec3(sCameraData.mNormal * vec4(vNormal, 0)));
    
    /** Adjuk tovább a világkoordinátabeli koordinátákat. */
    vPosVS = vec3(sCameraData.mModel * vec4(vPos, 1));
    
    /** Számoljuk ki a vertex pozícióját. */
    gl_Position = sCameraData.mProjection * sCameraData.mView * vec4(vPosVS, 1);
}