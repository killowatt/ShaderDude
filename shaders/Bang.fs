// http://www.pouet.net/prod.php?which=57245
#version 150 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float time;
uniform vec2 resolution;
uniform vec4 variables;
uniform vec4 triggers;

void main( ){
	vec3 c;
	float l,z=time * (1-variables[3]/150.0f);
	for(int i=0;i<3;i++) {
		vec2 uv,p=gl_FragCoord.xy/resolution;
		uv=p;
		p-=.5;
		p.x*=resolution.x/resolution.y;
		z+=variables[1]/10.0f;
		l=length(p);
		uv+=p/l*(sin(z)+1.)*abs(sin(l*9.-z*2.));
		c[i]=.01/length(abs(mod(uv,1.)-.5));
	}
  outColor=vec4(c/l,time);
}
