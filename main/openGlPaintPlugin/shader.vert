layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

out vec2 v_texCoord;
out vec3 theColor;

void main() {
	
	vec3 pos = inPosition;
	
	gl_Position = vec4(pos, 1.0);
	v_texCoord = inUV;
	theColor = inColor;
}