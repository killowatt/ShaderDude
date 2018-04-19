// Fractal base used taken from https://www.shadertoy.com/view/lslGWr and tweaked.
// Shader modified for SDE and is used for demonstration purposes only.

#version 150 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float time;
uniform vec2 resolution;
uniform vec4 variables;
uniform vec4 triggers;

uniform sampler2D diffuse;

float func(in vec3 p) 
{
    float sTime = time * .33;

	float str = 9.5;
	float acc, prev, tw = 0.;

	for (int i = 0; i < 32; ++i) 
    {
		float mag = dot(p, p);
		p = abs(p) / mag + vec3(vec2(0.4 * cos(sTime)), 0.4 * sin(sTime)) - 0.5;
		float w = exp( -float(i) / 5.);
		acc += w * exp(-str * pow(abs(mag - prev), 2.3));
		tw += w;
		prev = mag;
	}
    
	return max(0., 5. * acc / tw - .7);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - .5 * resolution.xy ) / resolution.y;
    vec3 p = vec3(uv, 0);
    float t = func(p);
    float t1 = func(p * 2.5);
    
	vec3 color = vec3(t, t * t, t * t * t) * vec3(t1 * t1 * t1, t1 * t1, t1 );
    
    // Output to screen
    outColor = vec4(color, 1.0);
}