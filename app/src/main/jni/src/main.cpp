#include <SDL.h>
#include <SDL_image.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3platform.h>
#include <fstream>
#include <math.h>

#define RATIO GLfloat(ScreenSize.w) / GLfloat(ScreenSize.h)

void RunSystem();
bool InitSDL();
bool InitWindow();
bool Init();
bool Input(int&, GLfloat&, GLfloat&, GLfloat&);
bool CompileShaders();

bool AisB(double a, double b, double error) { return a >= (b - error) && a <= (b + error); }

SDL_Window* mainWindow;
SDL_Rect ScreenSize = {0, 0, 1024, 720};
GLuint mainProgram;


void RunSystem() {
    if (!Init())
        return;
    glUseProgram(mainProgram);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    int rotCounter = 0;
    GLfloat x = 0.f, y = 0.f, scale = 1.f;
    while (Input(rotCounter, x, y, scale)) {
        static GLfloat rotation = 0.f;

        if (rotCounter > 0) {
            rotation -= M_PI_2 / 10.f;
            rotCounter -= 1;
        }
        else if (rotCounter < 0) {
            rotation += M_PI_2 / 10.f;
            rotCounter += 1;
        }
        GLfloat rotMatrix[] = {cosf(rotation), sinf(rotation), -sinf(rotation), cosf(rotation)};

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2f(glGetUniformLocation(mainProgram, "modelOffset"), x, -y);
        glUniform1f(glGetUniformLocation(mainProgram, "modelScale"), scale);
        glUniformMatrix2fv(glGetUniformLocation(mainProgram, "modelRotation"), 1, GL_FALSE, rotMatrix);
        glUniform1f(glGetUniformLocation(mainProgram, "ratio"), RATIO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        SDL_GL_SwapWindow(mainWindow);
    }
}

bool Input(int &rotCounter, GLfloat &x, GLfloat &y, GLfloat &scale) {
    static SDL_Event e;
    while(SDL_PollEvent(&e)) {
        static GLfloat downX, downY;
        static bool fingerIsDown = false;
        if (e.type == SDL_FINGERDOWN) {
            if (e.tfinger.y <= 0.5) {
                downX = e.tfinger.x;
                downY = e.tfinger.y;
                fingerIsDown = true;
            }
        }
        if (e.type == SDL_FINGERMOTION && !fingerIsDown) {
            x += e.tfinger.dx * 1.5f;
            y += e.tfinger.dy * 1.5f;
        }
        if (e.type == SDL_FINGERUP && fingerIsDown) {
            if (AisB(e.tfinger.y, downY, 0.2)) {
                if (e.tfinger.x > downX + 0.1)
                    rotCounter += 10;
                else if (e.tfinger.x < downX - 0.1)
                    rotCounter -= 10;
            }
            if (AisB(e.tfinger.x, downX, 0.2)) {
                if (e.tfinger.y > downY + 0.1)
                    scale -= 0.2;
                else if (e.tfinger.y < downY - 0.1)
                    scale += 0.2;
            }

            fingerIsDown = false;
        }

        if (e.type == SDL_QUIT)
            return false;
    }
    return true;
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
    const GLchar* fshader = "#version 300 es\n"
            "precision mediump float;\n"
            "\n"
            "in vec3 Color;\n"
            "out vec4 color;\n"
            "\n"
            "void main() {\n"
            "\tcolor = vec4(Color, 1.0);\n"
            "}\0";
    const GLchar* vshader = "#version 300 es\n"
            "layout(location = 0) in vec2 vpos;\n"
            "layout(location = 1) in vec3 color;\n"
            "\n"
            "uniform vec2 modelOffset;\n"
            "uniform float modelScale;\n"
            "uniform mat2 modelRotation;\n"
            "uniform float ratio;\n"
            "\n"
            "out vec3 Color;\n"
            "\n"
            "void main() {\n"
            "\tvec2 finalPos = modelRotation * (modelScale * vpos) + modelOffset;\n"
            "\tColor = color;\n"
            "\tfinalPos.y *= ratio;\n"
            "\tgl_Position = vec4(finalPos, 0.0, 1.0);\n"
            "}\0";

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

bool Init() {
    if (!InitSDL())
        return false;

    if (!InitWindow())
        return false;

    if (!CompileShaders())
        return false;

    GLfloat quad[] = {
            -0.5f, -0.5f,  0.9725, 0.0470, 0.0706,
            0.5f, -0.5f,  0.9960, 0.6823, 0.1764,
            0.5f,  0.5f,  0.4117, 0.8156, 0.1450,
            -0.5f,  0.5f,  0.2000, 0.0666, 0.7333
    };

    GLuint vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    return true;
}

int main(int argc, char* args[]) {
    RunSystem();

    return 0;
}

