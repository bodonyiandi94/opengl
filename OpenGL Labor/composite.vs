#version 430

/** Attribútum indexek. */
#define VERTEX_ATTRIB_POS    0
#define VERTEX_ATTRIB_UV     1

/** Input attribútumok. */
layout (location = VERTEX_ATTRIB_POS) in vec3 vPos;
layout (location = VERTEX_ATTRIB_UV) in vec2 vUV;

/** Output attribútumok. */
out vec2 vUVVS;

void main()
{
    /** Adjuk tovább az UV koordinátát. */
    vUVVS = vUV;
    
    /** Egyszerűen írjuk ki a csúcs koordinátáját, már
        eleve ablak koordinátákként adtuk meg őket. */
    gl_Position = vec4(vPos, 1);
}