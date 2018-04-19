// Shader modified for SDE and is used for demonstration purposes only.
// https://www.shadertoy.com/view/llsGW7

//// [2TC 15] Mystery Mountains.
// David Hoskins.

#version 150 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float time;
uniform vec2 resolution;
uniform vec4 variables;
uniform vec4 triggers;

uniform sampler2D diffuse;

// Add layers of the texture of differing frequencies and magnitudes...
#define F +(texture(diffuse,.3+p.xz*s/3e3)/(s+=s)+((triggers[2]/10.0)))
void main()
{
    vec2 w = gl_FragCoord.xy;
    vec4 p=vec4(w/resolution.xy,1,1)-.5,d=p,t;
    p.z += time*20.;d.y-=.4;
    for(float i=1.5;i>0.;i-=.002)
    {
        float s=.5;
        t = F F F F F F;
        outColor = vec4(0.8f+variables[0],25/255.0,0.8f + variables[2],9.0)+d.x-t*i;
        if(t.x>p.y*.007+1.3)break;
        p += d;
    }
}
