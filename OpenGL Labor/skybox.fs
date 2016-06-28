#version 430

/** A uniform puffer indexek. */
#define UNIFORM_BUFFER_CAMERA 0
#define UNIFORM_BUFFER_LIGHT  1
#define UNIFORM_BUFFER_OBJECT 2

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
in vec3 vPosVS;

/** Egyetlen pufferbe írunk csak, amely az ablakunkban jelenik
    meg, így egyetlen kimeneti attribútumunk van. */
out vec4 outColor;

/** Az objektumon alkalmazni kívánt textúra. */
layout (binding = 0) uniform samplerCube sTexture;

void main()
{
	vec3 uv = normalize(vPosVS);
	
    /** Vegyünk mintát a textúránkból. */
    vec3 textureColor = texture(sTexture, uv).rgb;
    //textureColor = normalize(abs(uv));
   
    /** A fragment színe az intenzitás és a csúcs színeként áll elõ. */
    outColor = vec4(textureColor, 1);
}