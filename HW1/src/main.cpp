// HW1: Interactive Antiprism Viewer
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include "InitShader.h"

// 창 제목은 "HW1_(학번)"이어야 한다.
#define STUDENT_ID "2020010437"

const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

const float PI = 3.14159265358979f;

// 도형 치수
const float RADIUS = 0.5f;       // 윗면/아랫면 정다각형의 외접원 반지름
const float HEIGHT = 0.8f;       // 윗면과 아랫면 사이의 거리
const float AXIS_LENGTH = 1.0f;  // 축 선분의 반 길이

const int MinN = 3;
const int MaxN = 10;
const int MaxVertices = 12 * MaxN;  // 삼각형 4n개 = 윗면 n개 + 아랫면 n개 + 옆면 2n개

// 윗면의 회전 각도 = Twist * PI / (TwistSteps * n)
// Twist의 범위는 (-TwistSteps, TwistSteps]이다. 그러므로 각도의 범위는 (-PI/n, PI/n]이다.
// C 키를 TwistSteps번 누르면 각기둥이 엇각기둥이 된다.
const int TwistSteps = 5;

const float SpinSpeed = 60.0f;  // 자전 속도 (degree/s)

// 확대/축소 비율의 범위
// 외접구가 클립 공간 [-1, 1]^3 안에 있으면, 도형은 어느 방향으로 회전해도 잘리지 않는다.
const float ScaleStep = 1.1f;
const float MinScale = 0.3f;
const float MaxScale = 1.0f / std::sqrt(RADIUS * RADIUS + 0.25f * HEIGHT * HEIGHT);

enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int     Axis = Xaxis;
GLfloat Theta[NumAxes] = { 0.0, 0.0, 0.0 };
bool    Spin = false;
float   Scale = 1.0f;

int  N = MinN;              // 윗면/아랫면의 변의 개수
int  Twist = 0;             // 윗면의 회전 단계
bool GeometryDirty = true;  // true이면 다음 프레임에서 정점을 다시 만든다.

glm::vec3 points[MaxVertices];
glm::vec3 colors[MaxVertices];
int NumVertices = 0;

GLuint shapeVao, shapeVbo, axisVao;
GLint  thetaLoc, scaleLoc;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

float twistAngle()
{
    return Twist * PI / (TwistSteps * N);
}

void addTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c,
                 const glm::vec3 &color)
{
    points[NumVertices] = a;  colors[NumVertices] = color;  NumVertices++;
    points[NumVertices] = b;  colors[NumVertices] = color;  NumVertices++;
    points[NumVertices] = c;  colors[NumVertices] = color;  NumVertices++;
}

// 옆면 2n개에 색상환(hue)을 같은 간격으로 나누어 준다.
glm::vec3 sideColor(int face, int n)
{
    return glm::rgbColor(glm::vec3(360.0f * face / (2 * n), 1.0f, 1.0f));
}

// n각 엇각기둥의 삼각형을 만든다. phi는 아랫면에 대한 윗면의 회전 각도이다.
// 모든 삼각형은 바깥에서 볼 때 반시계 방향(CCW)이다.
void buildAntiprism(int n, float phi)
{
    const glm::vec3 capColor(0.25f, 0.25f, 0.25f);
    const float y0 = -0.5f * HEIGHT;
    const float y1 = 0.5f * HEIGHT;

    // 각도가 커지면 꼭짓점은 +x에서 +z 쪽으로 움직인다. 이것은 위에서 볼 때 시계 방향이다.
    glm::vec3 B[MaxN], T[MaxN];
    for (int k = 0; k < n; k++) {
        float a = 2.0f * PI * k / n;
        B[k] = glm::vec3(RADIUS * std::cos(a),       y0, RADIUS * std::sin(a));
        T[k] = glm::vec3(RADIUS * std::cos(a + phi), y1, RADIUS * std::sin(a + phi));
    }
    const glm::vec3 bottomCenter(0.0f, y0, 0.0f);
    const glm::vec3 topCenter(0.0f, y1, 0.0f);

    NumVertices = 0;
    for (int k = 0; k < n; k++) {
        int k1 = (k + 1) % n;
        addTriangle(bottomCenter, B[k], B[k1], capColor);
        addTriangle(topCenter, T[k1], T[k], capColor);

        // 옆면 사각형 B[k] B[k1] T[k1] T[k]는 평면이 아니다. 짧은 대각선으로 나누어야
        // 볼록 다면체가 된다. phi >= 0이면 B[k1]-T[k]가 짧고, phi < 0이면 B[k]-T[k1]이 짧다.
        if (phi >= 0.0f) {
            addTriangle(B[k], T[k], B[k1], sideColor(2 * k, n));
            addTriangle(T[k], T[k1], B[k1], sideColor(2 * k + 1, n));
        } else {
            // 색상 번호를 한 칸 옮긴다. 그러면 PI/n에서 -PI/n으로 넘어갈 때
            // (두 모양은 같다) 각 면의 색상이 바뀌지 않는다.
            addTriangle(B[k], T[k1], B[k1], sideColor(2 * k, n));
            addTriangle(T[k], T[k1], B[k], sideColor(2 * ((k + n - 1) % n) + 1, n));
        }
    }
}

void updateGeometry()
{
    buildAntiprism(N, twistAngle());

    glBindBuffer(GL_ARRAY_BUFFER, shapeVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NumVertices * sizeof(glm::vec3), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), NumVertices * sizeof(glm::vec3), colors);
}

void printStatus()
{
    printf("n = %d, twist = %+6.1f deg (limit: +-%.1f deg), scale = %.2f\n",
           N, twistAngle() * 180.0f / PI, 180.0f / N, Scale);
    fflush(stdout);
}

void printUsage()
{
    printf("C / V   : rotate top face clockwise / counterclockwise\n"
           "O / P   : increase / decrease the number of sides (3 ~ 10)\n"
           "Space   : start / stop spin\n"
           "Mouse   : select spin axis (left: X, middle: Y, right: Z)\n"
           "I       : reset rotation\n"
           "Z / X   : scale down / up\n"
           "Esc     : quit\n\n");
}

void init()
{
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    GLuint vColor = glGetAttribLocation(program, "vColor");
    thetaLoc = glGetUniformLocation(program, "theta");
    scaleLoc = glGetUniformLocation(program, "scale");

    // 엇각기둥: n이 최대일 때의 크기로 버퍼를 한 번 할당한다.
    // 모양이 바뀌면 updateGeometry()가 glBufferSubData()로 내용만 바꾼다.
    glGenVertexArrays(1, &shapeVao);
    glBindVertexArray(shapeVao);

    glGenBuffers(1, &shapeVbo);
    glBindBuffer(GL_ARRAY_BUFFER, shapeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));

    // 도형의 중심을 지나는 축: X는 빨강, Y는 초록, Z는 파랑
    const glm::vec3 axisPoints[6] = {
        glm::vec3(-AXIS_LENGTH, 0.0f, 0.0f), glm::vec3(AXIS_LENGTH, 0.0f, 0.0f),
        glm::vec3(0.0f, -AXIS_LENGTH, 0.0f), glm::vec3(0.0f, AXIS_LENGTH, 0.0f),
        glm::vec3(0.0f, 0.0f, -AXIS_LENGTH), glm::vec3(0.0f, 0.0f, AXIS_LENGTH)
    };
    const glm::vec3 axisColors[6] = {
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)
    };

    glGenVertexArrays(1, &axisVao);
    glBindVertexArray(axisVao);

    GLuint axisVbo;
    glGenBuffers(1, &axisVbo);
    glBindBuffer(GL_ARRAY_BUFFER, axisVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisPoints) + sizeof(axisColors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(axisPoints), axisPoints);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(axisPoints), sizeof(axisColors), axisColors);

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(axisPoints)));

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

void update(float dt)
{
    if (Spin) {
        Theta[Axis] += SpinSpeed * dt;
        if (Theta[Axis] > 360.0f) Theta[Axis] -= 360.0f;
    }
    if (GeometryDirty) {
        updateGeometry();
        GeometryDirty = false;
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3fv(thetaLoc, 1, Theta);
    glUniform1f(scaleLoc, Scale);

    glBindVertexArray(shapeVao);
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);

    glBindVertexArray(axisVao);
    glDrawArrays(GL_LINES, 0, 6);
}

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "HW1_" STUDENT_ID, NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    printf("OpenGL %s, GLSL %s\n\n", glGetString(GL_VERSION),
                                     glGetString(GL_SHADING_LANGUAGE_VERSION));
    printUsage();

    init();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);
    printStatus();

    double prevTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        update(float(now - prevTime));
        prevTime = now;

        display();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

// 윗면을 d 단계만큼 회전한다. (-TwistSteps, TwistSteps] 범위를 넘으면 반대쪽 끝으로 간다.
// 각도 PI/n과 -PI/n은 같은 모양이므로, 윗면은 끊기지 않고 계속 회전한다.
void twist(int d)
{
    const int period = 2 * TwistSteps;
    Twist = ((Twist + d + TwistSteps - 1) % period + period) % period - (TwistSteps - 1);
    GeometryDirty = true;
}

// 키를 누르고 있을 때 동작이 반복되지 않도록 폴링(glfwGetKey) 대신 콜백을 사용한다.
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_C:
        twist(+1);
        break;
    case GLFW_KEY_V:
        twist(-1);
        break;
    case GLFW_KEY_O:
        if (N < MaxN) { N++; GeometryDirty = true; }
        break;
    case GLFW_KEY_P:
        if (N > MinN) { N--; GeometryDirty = true; }
        break;
    case GLFW_KEY_SPACE:
        Spin = !Spin;
        return;
    case GLFW_KEY_I:
        Theta[Xaxis] = Theta[Yaxis] = Theta[Zaxis] = 0.0f;
        return;
    case GLFW_KEY_Z:
        Scale = std::max(MinScale, Scale / ScaleStep);
        break;
    case GLFW_KEY_X:
        Scale = std::min(MaxScale, Scale * ScaleStep);
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        return;
    default:
        return;
    }
    printStatus();
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:   Axis = Xaxis; break;
    case GLFW_MOUSE_BUTTON_MIDDLE: Axis = Yaxis; break;
    case GLFW_MOUSE_BUTTON_RIGHT:  Axis = Zaxis; break;
    }
}

// 창이 정사각형이 아니어도 도형이 찌그러지지 않도록, 가운데에 정사각형 viewport를 둔다.
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    int size = std::min(width, height);
    glViewport((width - size) / 2, (height - size) / 2, size, size);
}
