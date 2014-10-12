#version 150

in vec4 vPosition;
in int vState;
uniform int kE;
uniform int kI;
uniform mat4 projection;
out vec4 fColor;
void main() 
{
	gl_PointSize = 4.0;
	gl_Position = projection * vPosition / vPosition.w;
	if (vState == 0) {
		fColor = vec4(0, 1, 0, 1);
	} else if (vState == -1) {
		fColor = vec4(0, 0, 0, 1);
	} else if (vState == -2) {
		fColor = vec4(0, 0, 1, 1);
	} else if (vState <= kE) {
		fColor = vec4(1, 1, 0, 1);
	} else { // vState <= (kE + kI)
		fColor = vec4(1, 0, 0, 1);
	}
}

