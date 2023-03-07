#version 150

in vec4 vPosition;
in ivec3 vState;
uniform mat4 projection;
out vec4 fColor;

vec4 colorS = vec4(0, 0, 1, 1);
vec4 colorE = vec4(1, 1, 0, 1);
vec4 colorI = vec4(1, 0, 0, 1);
vec4 colorR = vec4(0, 1, 0, 1);
vec4 colorD = vec4(0, 0, 0, 1);

void main() 
{
	gl_Position = projection * vPosition / vPosition.w;

	if (vState[2] == 0) {
		fColor = colorS;
	} else if (vState[2] == 1) {
		fColor = colorE;
	} else if (vState[2] == 2) {
		fColor = colorI;
	} else if (vState[2] == 4) { 
		fColor = colorR;
	} else if (vState[2] == 5) {
		fColor = colorD;
	} else {
		fColor = vec4(1, 1, 1, 1);
	}
}
