#version 330

in vec2 v_uv;

out vec4 fragColor;

uniform sampler2D u_texture; 

void main(void)
{
	vec3 texture_color = texture(u_texture, v_uv).xyz;

	// We're just going to paint the interpolated colour from the vertex shader
	fragColor =  vec4(texture_color, 1.0);
}
