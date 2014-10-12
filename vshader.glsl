#version 150

in vec4 vPosition;
in ivec3 vState;
uniform mat4 projection;
out vec4 fColor;
void main() 
{
	gl_Position = projection * vPosition / vPosition.w;

	if (vState[2] == 0) {
		fColor = vec4(0, 1, 0, 1);
	} else if (vState[2] == 1) {
		fColor = vec4(1, 1, 0, 1);
	} else if (vState[2] == 2) {
		fColor = vec4(1, 0, 0, 1);
	} else if (vState[2] == 4) { 
		fColor = vec4(0, 0, 1, 1);
	} else if (vState[2] == 5) {
		fColor = vec4(0, 0, 0, 1);
	} else {
		fColor = vec4(1, 1, 1, 1);
	}
}

