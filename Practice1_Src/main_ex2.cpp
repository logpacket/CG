#include <vgl.h>
#include <InitShader.h>
#include <mat.h>
#include <cstdio>
#include <cstdlib>
//

const int NumTimesToSubdivide = 4;
const int NumTetrahedra = 256; //4^4
const int NumTriangles = 4 * NumTetrahedra;
const int NumVertices = 3 * NumTriangles;

vec3 points[NumVertices];
vec3 colors[NumVertices];

int Index = 0;

const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// Create a triangle from three vertices 
void triangle(const vec3 &a, const vec3 &b, const vec3 &c, const int color)
{
    static vec3  base_colors[] = {
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 0.0, 0.0)
    };

    points[Index] = a;  colors[Index] = base_colors[color];  Index++;
    points[Index] = b;  colors[Index] = base_colors[color];  Index++;
    points[Index] = c;  colors[Index] = base_colors[color];  Index++;
}

// Create a tetrahedron from 4 triangles
void tetra( const vec3& a, const vec3& b, const vec3& c, const vec3& d )
{
    triangle( a, b, c, 0 );
    triangle( a, c, d, 1 );
    triangle( a, d, b, 2 );
    triangle( b, d, c, 3 );
}

// Divide a tetrahedron recursively
void divide_tetra( const vec3& a, const vec3& b,
	      const vec3& c, const vec3& d, int count )
{
    if ( count > 0 ) {
        vec3 v0 = ( a + b ) / 2.0;
        vec3 v1 = ( a + c ) / 2.0;
        vec3 v2 = ( a + d ) / 2.0;
	vec3 v3 = ( b + c ) / 2.0;
	vec3 v4 = ( c + d ) / 2.0;
	vec3 v5 = ( b + d ) / 2.0;
        divide_tetra( a, v0, v1, v2, count - 1 );
        divide_tetra( v0, b, v3, v5, count - 1 );
        divide_tetra( v1, v3, c, v4, count - 1 );
	divide_tetra( v2, v4, v5, d, count - 1 );
    }
    else {
        tetra( a, b, c, d );    
    }
}


void init( void )
{
    vec3 vertices[4] = {
	vec3( 0.0, 0.0, -1.0 ),
	vec3( 0.0, 0.942809, 0.333333 ),
	vec3( -0.816497, -0.471405, 0.333333 ),
	vec3( 0.816497, -0.471405, 0.333333 )
    };	
    divide_tetra( vertices[0], vertices[1], vertices[2], vertices[3],
		  NumTimesToSubdivide );

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);


    //load shaders
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    //initialize vertex position attribute from vertex shader
    GLuint vPos = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPos);
    glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));


    GLuint vCol = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vCol);
    glVertexAttribPointer( vCol, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(points)) );

    glEnable(GL_DEPTH_TEST);
   glClearColor(1.0, 1.0, 1.0, 1.0);
}

void display()
{
    glClear( GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT );
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
    glFlush();
}


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // for core-profile mode
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // for compatibility mode
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Many Points On GPU", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    init();

    while (!glfwWindowShouldClose(window)) {
        // input
        processInput(window);

        // render
        display();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}