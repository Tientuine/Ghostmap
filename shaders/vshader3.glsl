#version 150

in vec4 vPosition;
in ivec3 vState;
uniform mat4 projection;
out vec4 fColor;
void main() 
{
	//gl_PointSize = 4.0;
	gl_Position = projection * vPosition / vPosition.w;
	//fColor = vec4( vState[2] * 10 / 255.0, vState[2] * 10 / 255.0, vState[2] * 10 / 255.0, 1);
	//fColor = vec4(1, 0, 0, 1);
	//fColor = vec4(1, 0, 1, 1);

	if (vState[2] == 0) {
		fColor = vec4(0, 1, 0, 1);
	} else if (vState[2] == -2) { //vState[0] == -1073741824) { // why this value for -2?
		fColor = vec4(0, 0, 1, 1);
	} else if (vState[2] == -1) { //vState[0] == -1082130432) { // why this value for -1?
		fColor = vec4(0, 0, 0, 1);
	} else if (0 < vState[2] && vState[2] <= vState[1]) {
		fColor = vec4(1, 1, 0, 1);
	} else if (vState[2] <= (vState[1] + vState[0])) {
		fColor = vec4(1, 0, 0, 1);
	}
}

