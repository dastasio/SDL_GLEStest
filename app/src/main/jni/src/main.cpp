/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL, standard IO, and, strings
#include <SDL.h>
#include <SDL_image.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3platform.h>
#include <fstream>
#include <math.h>

void RunSystem();
bool InitSDL();
bool InitWindow();
bool CompileShaders();


SDL_Window* mainWindow;
SDL_Rect ScreenSize = {0, 0, 1024, 720};
GLuint mainProgram;

GLfloat f(double x) {
    return (cosf(6.3 * x) + 1) / 2;
}

void RunSystem() {
    if (!InitSDL())
        return;

    if (!InitWindow())
        return;

    if (!CompileShaders())
        return;

    GLfloat quad[] = {
            -0.5, -0.5,
             0.5, -0.5,
             0.5,  0.5,
            -0.5,  0.5
    };

    GLuint vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(mainProgram);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        static GLfloat x = 0.f, y = 0.f;
        while(SDL_PollEvent(&e)) {
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
                x = e.tfinger.x * 2 - 1.f;
                y = e.tfinger.y * 2 - 1.f;
            }

            if (e.type == SDL_QUIT)
                quit = true;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2f(glGetUniformLocation(mainProgram, "modelOffset"), x, -y);
        glUniform1f(glGetUniformLocation(mainProgram, "modelScale"), 0.2f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        SDL_GL_SwapWindow(mainWindow);
    }

    SDL_Log("[INFO] Successfully executed app. Now closing!");
}

bool InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("[ERROR] Could not initalize SDL2:\n%s\n", SDL_GetError());
        return false;
    }

    int flags = IMG_INIT_PNG;
    if ((!IMG_Init(flags)) & flags) {
        SDL_Log("[ERROR] Could not initalize SDL2_image:\n%s\n", IMG_GetError());
        return false;
    }
    return true;
}

bool InitWindow() {
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "l")) {
        SDL_Log("[WARNING] Linear texture filtering unsupported!\n");
    }

    /* getting display size */
    SDL_DisplayMode displayInfo;
    if (SDL_GetCurrentDisplayMode(0, &displayInfo) == 0) {
        ScreenSize.w = displayInfo.w;
        ScreenSize.h = displayInfo.h;
    }

    /* setting OpenGL context */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* creating window */
    mainWindow = SDL_CreateWindow("SDL GL Android App",
                                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                              ScreenSize.w, ScreenSize.h,
                                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (mainWindow == NULL) {
        SDL_Log("[ERROR] Could not create window:\n%s\n", SDL_GetError());
        return false;
    }

    /* creating context */
    SDL_GLContext mainContext = SDL_GL_CreateContext(mainWindow);
    if (mainContext == NULL) {
        SDL_Log("[ERROR] Could not create OpenGL context:\n%s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(mainWindow, mainContext);
    glViewport(0, 0, ScreenSize.w, ScreenSize.h);
    return true;
}

bool CompileShaders() {
    const GLchar* fshader =
            "#version 300 es          \n"
            "precision mediump float; \n"
            "out vec4 color;          \n"
            "void main() {            \n"
            "\tcolor = vec4(1.0);     \n"
            "}                        \n\0";
    const GLchar* vshader = "#version 300 es\n"
            "layout(location = 0) in vec2 vpos;\n"
            "\n"
            "uniform vec2 modelOffset;\n"
            "uniform float modelScale;\n"
            "\n"
            "void main() {\n"
            "\tvec2 finalPos = (modelScale * vpos) + modelOffset;\n"
            "\tgl_Position = vec4(finalPos, 0.0, 1.0);\n"
            "}\n"
            "\0";

    GLuint vsh, fsh;
    GLint success;

    vsh = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsh, 1, &vshader, NULL);
    glCompileShader(vsh);
    glGetShaderiv(vsh, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(vsh, GL_INFO_LOG_LENGTH, &log_size);
        char* log = new char[log_size];
        glGetShaderInfoLog(vsh, log_size, NULL, log);
        SDL_Log("[ERROR] Could not compile vertex shader:\n%s\n", log);
        return false;
    }

    fsh = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsh, 1, &fshader, NULL);
    glCompileShader(fsh);
    glGetShaderiv(fsh, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(fsh, GL_INFO_LOG_LENGTH, &log_size);
        char* log = new char[log_size];
        glGetShaderInfoLog(fsh, log_size, NULL, log);
        SDL_Log("[ERROR] Could not compile fragment shader:\n%s\n", log);
        return false;
    }


    mainProgram = glCreateProgram();
    glAttachShader(mainProgram, vsh);
    glAttachShader(mainProgram, fsh);
    glLinkProgram(mainProgram);
    glGetProgramiv(mainProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(mainProgram, GL_INFO_LOG_LENGTH, &log_size);
        char* log = new char[log_size];
        glGetProgramInfoLog(mainProgram, log_size, NULL, log);
        SDL_Log("[ERROR] Could not link program:\n%s\n", log);
        exit(EXIT_FAILURE);
    }

    return true;
}

int main(int argc, char* args[]) {
    RunSystem();

    return 0;
}

