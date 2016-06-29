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

/** Egy irányított fényforráshoz tartozó adatok. */
struct DirectionalLightData
{
    vec3 vLightDirWS;
    vec3 vLightColor;
};

/** Egy pontszerű fényforráshoz tartozó adatok. */
struct PointLightData
{
    vec3 vLightPosWS;
    vec3 vDiffuseColor;
    vec3 vSpecularColor;
    vec3 vAmbientColor;
    float fRadius;
};

/** Fényforrás uniform puffere. */
layout (std140, binding = UNIFORM_BUFFER_LIGHT) uniform LightData
{
    PointLightData sData;
} sLightData;

/** Input attribútumok. */
in vec2 vUVVS;
in vec3 vNormalVS;
in vec3 vPosVS;

/** Egyetlen pufferbe írunk csak, amely az ablakunkban jelenik
    meg, így egyetlen kimeneti attribútumunk van. */
out vec4 outColor;

/** Az objektumon alkalmazni kívánt textúra. */
layout (binding = 0) uniform sampler2D sTexture;

/** Kiszámítja egy irányított fényforrás mennyiségét a megadtott adatokkal. */
vec3 calcDirectionalLight(DirectionalLightData lightData, vec3 vPosWS, vec3 vNormalWS)
{
    /** A fény intenzitása egy skalárszorzattal. */
    float fLightIntensity = max(dot(vNormalWS, lightData.vLightDirWS), 0.0);
    
    /** A végeredmény az intenzitás és a fény színének szorzata. */
    return lightData.vLightColor * fLightIntensity;
}

/** Kiszámítja egy pontszerű fényforrás mennyiségét a megadtott adatokkal. */
vec3 calcPointLight(PointLightData lightData, vec3 vPosWS, vec3 vNormalWS)
{
    /** Számítsuk ki az adott pixeltől a fényforrás felé mutató vektort. */
    vec3 toLight = lightData.vLightPosWS.xyz - vPosWS;
    
    /** Ennek a hossza a pixel és a fényforrás távolsága. */
    float dist = length(toLight);
    
    /** Ha a távolság nagyobb, mint a sugár, akkor nulla a fény mennyisége. */
    if (dist > lightData.fRadius)
        return vec3(0.0);
    
    /** Normalizáljuk az előző vektort. */
    toLight = toLight / dist;
    
    /** A kamerába mutató vektort. */
    vec3 toEye = normalize(sCameraData.vEye - vPosWS.xyz);
    
    /** A fényfelé mutató vektor tükrözöttje a normáltra. */
    vec3 reflected = normalize(reflect(-toLight, vNormalWS));
    
    /** A fény csillapodásának mértéke (a fényforrás felé távolodva a fény mennyisége csökken). */
	float attenuation = clamp(1.0 - dist * dist / (lightData.fRadius * lightData.fRadius), 0.0, 1.0); 
	attenuation *= attenuation;

    /** A diffúz együttható a normálvektor és a fény felé mutató vektor skalárszorzata. */
    float diffuseIntensity = max(dot(vNormalWS, toLight), 0.0);
    
    /** A csillanás mértéke a visszavert, illetve a kamerába mutató vektor skalárszorzatakén áll elő. */
    float specularIntensity = pow(max(dot(toEye, reflected), 0.0), 32);
	
	vec3 returnColor = 
		diffuseIntensity *lightData. vDiffuseColor + 
		specularIntensity * lightData.vSpecularColor + 
		lightData.vAmbientColor;
   
    /** A végeredmény a fény színének és az együtthatóknak a szorzata, csillapítva. */
    return returnColor * attenuation;
}

void main()
{
    /** Normalizáljuk újra a normálvektort. (A 'WS' a 'world space' rövidítést jelöli,
        azt jelenti, hogy a normálvektor a világ koordinátarendszerében van. */
    vec3 vNormalWS = normalize(vNormalVS);
    
    /** Számoljuk ki a megvilágítás mértékét. */
    vec3 vLightColor = calcPointLight(sLightData.sData, vPosVS, vNormalWS);
    
    /** Vegyünk mintát a textúránkból. */
    vec3 textureColor = texture2D(sTexture, vUVVS).rgb;
   
    /** A fragment színe az intenzitás és a csúcs színeként áll elő. */
    outColor = vec4(vLightColor * textureColor, 1);
}