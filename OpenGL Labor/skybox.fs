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

/** Input attrib�tumok. */
in vec3 vPosVS;

/** Egyetlen pufferbe �runk csak, amely az ablakunkban jelenik
    meg, �gy egyetlen kimeneti attrib�tumunk van. */
out vec4 outColor;

/** Az objektumon alkalmazni k�v�nt text�ra. */
layout (binding = 0) uniform samplerCube sTexture;

void main()
{
	vec3 uv = normalize(vPosVS);
	
    /** Vegy�nk mint�t a text�r�nkb�l. */
    vec3 textureColor = texture(sTexture, uv).rgb;
    //textureColor = normalize(abs(uv));
   
    /** A fragment sz�ne az intenzit�s �s a cs�cs sz�nek�nt �ll el�. */
    outColor = vec4(textureColor, 1);
}