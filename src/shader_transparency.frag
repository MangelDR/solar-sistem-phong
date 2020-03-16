#version 330

in vec2 v_uv;

out vec4 fragColor;

uniform sampler2D u_texture; 
uniform float u_transparency;

void main(void)
{
	vec3 texture_color = texture(u_texture, v_uv).xyz;
	vec4 color = vec4 (vec3(1.0 - texture_color.r, 1.0 - texture_color.g, 1.0 - texture_color.b), u_transparency);
	if(color.a < 0.1) discard;

	// We're just going to paint the interpolated colour from the vertex shader
	fragColor =  color;
}
