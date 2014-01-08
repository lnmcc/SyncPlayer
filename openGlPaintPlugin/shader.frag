in vec3 theColor;

uniform sampler2D u_texture1;
uniform sampler2D u_texture2;

int vec2 v_texCoord;
out vec4 outputColor;

void main() {

	vec4 color1 = texture(u_texture1, v_texCoord);
	vec4 colot2 = texture(u_texture2, v_texCoord);
	
	outputColor = color1 + color2;
}