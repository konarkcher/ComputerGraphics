// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>

using namespace glm;

#include <common/shader.hpp>

int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Homework 1 - The Legend of Zelda", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create and compile our GLSL program from the shaders
    GLuint leftProgramID = LoadShaders("SimpleVertexShader.vertexshader", "OuterFragmentShader.fragmentshader");
    GLuint rightProgramID = LoadShaders("SimpleVertexShader.vertexshader", "InnerFragmentShader.fragmentshader");

    static const GLfloat g_vertex_buffer_data[] = {
            -0.8f, -0.8f, 0.0f,
            0.0f, 0.8f, 0.0f,
            0.8f, -0.8f, 0.0f,

            -0.5f, 0.0f, 0.0f,
            0.0f, -0.9f, 0.0f,
            0.5f, 0.0f, 0.0f,
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    do {

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*) 0            // array buffer offset
        );

        // Use our shader
        glUseProgram(leftProgramID);

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(rightProgramID);
        glDrawArrays(GL_TRIANGLES, 3, 6);

        glDisableVertexAttribArray(0);
        // glDisableVertexAttribArray(vertexPosition_modelspaceID);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);


    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteProgram(leftProgramID);
    glDeleteProgram(rightProgramID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

