// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#include <assert.h>
#include <stdio.h>

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

//#define  WGL_SWAP_METHOD_ARB WGL_SWAP_EXCHANGE_ARB

#define bool _Bool
#define false 0
#define true 1

SDL_Window* window;
SDL_GLContext glContext;

GLuint unit_shader;
GLuint unit_vao;
GLuint vertex_vbo;
GLuint color_vbo;

uint32_t width, height;

const GLchar* vertexSource = "#version 300 es\n"
                             "precision mediump float;"
                             "layout(location = 0) in vec3 position;"
                             "layout(location = 1) in vec3 color;\n"
                             "smooth out vec4 Color;\n"
                             "void main()\n"
                             "{\n"
                             "    gl_Position = vec4(position, 1.0);\n"
                             "    Color = vec4(color,1);\n"
                             "}\n";

const GLchar* fragmentSource = "#version 300 es\n"
                               "precision mediump float;\n"
                               "smooth in vec4 Color;\n"
                               "out vec4 outColor;\n"
                               "void main()\n"
                               "{\n"
                               "	outColor = Color;\n"
                               "}\n";

bool valid_shader(GLint shader)
{
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        GLchar* errorLog = (GLchar*)malloc(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
        printf("shader error: %s\n", &errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader);    // Don't leak the shader.
        free(errorLog);
        return false;
    }
    else
        return true;
}

GLuint compile_program()
{
    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    assert(valid_shader(vertexShader));

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    assert(valid_shader(fragmentShader));

    // Link the vertex and fragment shader into a shader program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glBindFragDataLocation(program, 0, "outColor");
    glLinkProgram(program);
    glUseProgram(program);
    assert(program != 0);

    return program;
}

int initWp()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
        return 1;

    // SDL_DisplayMode displayMode;
    // SDL_GetCurrentDisplayMode(0, &displayMode);
    // width = displayMode.w;
    // height = displayMode.h;
    width  = 512;
    height = 512;

    window = SDL_CreateWindow("sonar", 0, 0,
        // SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        // SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        // displayMode.w, displayMode.h,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP);
    // SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

    int idx = SDL_GetWindowDisplayIndex(window);
    SDL_Rect bounds;
    SDL_GetDisplayBounds(idx, &bounds);
    SDL_SetWindowBordered(window, SDL_FALSE);
    SDL_SetWindowPosition(window, bounds.x, bounds.y);
    SDL_SetWindowSize(window, bounds.w, bounds.h);

    width  = bounds.w;
    height = bounds.h;

    glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL)
    {
        printf("There was an error creating the OpenGL context!\n");
        return 0;
    }

    SDL_SetWindowFullscreen(window, SDL_FALSE);

    const unsigned char* version = glGetString(GL_VERSION);
    // printf("%s\n", (const char*)version);
    if (version == NULL)
    {
        printf("There was an error creating the OpenGL context!\n");
        return 1;
    }

    // SDL_GL1MakeCurrent(window, glContext);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetSwapInterval(0);

    // MUST make a context AND make it current BEFORE glewInit()!
    glewExperimental   = GL_TRUE;
    GLenum glew_status = glewInit();
    if (glew_status != 0)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return 1;
    }

    unit_shader = compile_program();

    {
        // vbos and vbas
        glGenVertexArrays(1, &unit_vao);

        // vertex_vbo
        glGenBuffers(1, &vertex_vbo);
        // color_vbo
        glGenBuffers(1, &color_vbo);

        glBindVertexArray(unit_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
        GLint posAttrib = glGetAttribLocation(unit_shader, "position");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
        GLint colAttrib = glGetAttribLocation(unit_shader, "color");
        glEnableVertexAttribArray(colAttrib);
        glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    }

    glEnable(GL_DEPTH_TEST);

    // glViewport(0, 0, width, height);
    // glClearColor(0.0,0.0,0.0,1.0);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(3.0);

    return 0;
}

int updateWp(const float* vertices, const float* colors, int n_vertices)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(unit_shader);

    glBindVertexArray(unit_vao);
    // vertex_vbo
    glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * n_vertices * 3, vertices, GL_STREAM_DRAW);
    // color_vbo
    glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors[0]) * n_vertices * 3, colors, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_POINTS, 0, n_vertices);

    SDL_GL_SwapWindow(window);
    return 0;
}

int cleanupWp()
{
    SDL_GL_DeleteContext(glContext);
    SDL_Quit();

    return 0;
}
