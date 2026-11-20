#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D CC_Texture0;
varying vec2 v_texCoord;
uniform vec2 textureSize;
uniform float percentage;

void main() {
	const vec4 unplayColor = vec4(1.00, 0.68, 0.19, 1.0);

	float height = texture2D(CC_Texture0, vec2(v_texCoord.x, 0.0)).r;
	float mirrorY = abs(v_texCoord.y * 2.0 - 1.0);
	float barFactor = step(mirrorY, height);
	vec4 tex = vec4(vec3(barFactor), 1.0);

	const float off = 0.01;
	float fac = step(percentage, v_texCoord.x);
	vec4 playColor = vec4(vec3(smoothstep(percentage - 0.4, percentage, v_texCoord.x) * 0.2 + 0.8), 1.0);
	vec4 color = mix(playColor, unplayColor, fac);
	gl_FragColor = tex * color;
	// gl_FragColor = vec4(v_texCoord, 0.0, 1.0);
}