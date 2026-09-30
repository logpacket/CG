#version 330

in vec4 vPosition;
in vec3 vColor;
out vec4 color;

uniform vec3 theta;   // 각 축에 대한 회전 각도 (degree)
uniform float scale;  // 확대/축소 비율

// 처음 위치에서 윗면이 보이도록 보는 방향을 기울인다 (degree).
const vec2 viewTilt = vec2(25.0, 20.0);

mat4 rotateX(float a)
{
    float c = cos(a), s = sin(a);
    return mat4(1.0, 0.0, 0.0, 0.0,
                0.0,   c,   s, 0.0,
                0.0,  -s,   c, 0.0,
                0.0, 0.0, 0.0, 1.0);
}

mat4 rotateY(float a)
{
    float c = cos(a), s = sin(a);
    return mat4(  c, 0.0,  -s, 0.0,
                0.0, 1.0, 0.0, 0.0,
                  s, 0.0,   c, 0.0,
                0.0, 0.0, 0.0, 1.0);
}

mat4 rotateZ(float a)
{
    float c = cos(a), s = sin(a);
    return mat4(  c,   s, 0.0, 0.0,
                 -s,   c, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0);
}

void main()
{
    vec3 angles = radians(theta);
    mat4 model = rotateZ(angles.z) * rotateY(angles.y) * rotateX(angles.x);
    mat4 view = rotateX(radians(viewTilt.x)) * rotateY(radians(viewTilt.y));

    // glOrtho(-1, 1, -1, 1, -1, 1)과 같다. z의 부호를 바꾸어 +z가 화면 앞쪽을 향하게 한다.
    mat4 projection = mat4(1.0, 0.0,  0.0, 0.0,
                           0.0, 1.0,  0.0, 0.0,
                           0.0, 0.0, -1.0, 0.0,
                           0.0, 0.0,  0.0, 1.0);

    gl_Position = projection * view * model * vec4(scale * vPosition.xyz, 1.0);
    color = vec4(vColor, 1.0);
}
