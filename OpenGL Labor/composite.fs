#version 430

/** Input attribútumok. */
in vec2 vUVVS;

/** Egyetlen pufferbe írunk csak, amely az ablakunkban jelenik
    meg, így egyetlen kimeneti attribútumunk van. */
out vec4 outColor;

/** A framebufferünk textúrája. */
layout (binding = 0) uniform sampler2D sTexture;

void main()
{
    vec3 textureColor = texture2D(sTexture, vUVVS).rgb;
   
    /** A fragment színe az intenzitás és a csúcs színeként áll elő. */
    outColor = vec4(textureColor, 1);
}