// Bufferless text rendering demo for OpenGL
// https://github.com/sevmeyer/textshader
// SPDX-License-Identifier: 0BSD

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>


#define ASCII_MIN  0x20
#define ASCII_MAX  0x7E


static const GLuint BITMAPS[ASCII_MAX - ASCII_MIN + 1] = {
    0x00000000,0x08421004,0x14A50000,0x15F52BEA,0xBC1741E4,0x0D641358,
    0x1C1F252E,0x08420000,0x88421088,0x88421082,0x09F22800,0x00427C84,
    0x40003188,0x00007C00,0x0000018C,0x10821042,0x1D18C62E,0x10C52108,
    0x1D18383F,0x1F06422E,0x314947F0,0x3E17C22E,0x1C17C62E,0x3F042084,
    0x1D17462E,0x1D18FA0E,0x00C6018C,0x58C03188,0x00820888,0x000F83E0,
    0x00222082,0x1D083004,0xA3DAF42E,0x1D18FE31,0x1F17C62F,0x1D10862E,
    0x1F18C62F,0x3E17843F,0x3E10BC21,0x3C1EC63E,0x231FC631,0x3E42109F,
    0x3D08422E,0x22929D31,0x0210843F,0x23BAC631,0x233AE631,0x1D18C62E,
    0x1F18BC21,0x1D18D536,0x1F18BD31,0x3C17422E,0x3E421084,0x2318C62E,
    0x2318A944,0x231AD54A,0x22A21151,0x23151084,0x3E82083F,0x8421084E,
    0x04221108,0x9084210E,0x08A88000,0x4000001F,0x04440000,0x00E87A3E,
    0x02F8C62F,0x00E8862E,0x21E8C63E,0x00E8FC2E,0x1D10BC21,0x7D18FA0E,
    0x02F8C631,0x1007A108,0xE2E803D0,0x03149571,0x0E421098,0x00FAD6B5,
    0x00F8C631,0x00E8C62E,0x5F18BC21,0x7D18FA10,0x00F88421,0x01E0BA0F,
    0x02F0862E,0x0118C63E,0x0118A944,0x011AD54A,0x01151151,0x6318FA0E,
    0x01F4105F,0x8420884C,0x88421084,0x90882106,0x00095520};


static const char* VERT_SOURCE =
    "#version 330 core\n"
    "uniform vec2 uniPos;"
    "uniform vec2 uniScale;"
    "uniform uint uniChars[16];"
    "out vec2 coords;"
    "flat out uint bitmap;"
    "void main()"
    "{"
        "int vert = gl_VertexID % 6;"
        "int quad = gl_VertexID / 6;"
        "bitmap = uniChars[quad];"

        "float low = float(bitmap > 0x40000000u);"
        "float big = float(bitmap > 0x80000000u);"

        "float u = float((14u >> vert) & 1u);"
        "float v = float((28u >> vert) & 1u);"
        "float y = v*(6.0 + big);"

        "coords = vec2(u, y/6.0);"
        "vec2 pos = vec2(quad*6.0 + u*5.0, y - low);"
        "gl_Position = vec4(pos*uniScale + uniPos, 0.0, 1.0);"
    "}\n";


static const char* FRAG_SOURCE =
    "#version 330 core\n"
    "in vec2 coords;"
    "flat in uint bitmap;"
    "layout(location = 0) out vec4 color;"
    "void main()"
    "{"
        "float roll = float(bitmap > 0xC0000000u);"

        "uint row = (uint(coords.y*6.0 + roll*4.0) % 7u) % 6u;"
        "uint col = uint(coords.x*5.0);"
        "uint bit = 1u << (row*5u + col);"

        "if ((bitmap & bit) == 0u) discard;"
        "color = vec4(0.2, 0.2, 0.2, 1.0);"
    "}\n";


static SDL_Window* window;
static SDL_GLContext context;

static GLuint vao;
static GLuint program;
static GLint uniPos;
static GLint uniScale;
static GLint uniChars;


void GLAPIENTRY debugCallback(GLenum, GLenum, GLuint,
    GLenum, GLsizei, const GLchar* message, const void*)
{
    SDL_Log("OpenGL debug message: %s", message);
}


static GLuint compileProgram()
{
    glGenVertexArrays(1, &vao);

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &VERT_SOURCE, NULL);
    glCompileShader(vertShader);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &FRAG_SOURCE, NULL);
    glCompileShader(fragShader);

    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    uniPos = glGetUniformLocation(program, "uniPos");
    uniScale = glGetUniformLocation(program, "uniScale");
    uniChars = glGetUniformLocation(program, "uniChars");

    return vao && uniPos >= 0 && uniScale >= 0 && uniChars >= 0;
}


static int initWindow()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("textshader",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        960, 540, SDL_WINDOW_OPENGL);

    if (!window) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return 0;
    }

    context = SDL_GL_CreateContext(window);

    if (!context) {
        SDL_Log("Could not create OpenGL context: %s", SDL_GetError());
        return 0;
    }

    if (GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugCallback, NULL);
    }

    return compileProgram();
}


static void drawLabel(const char* text, int indent, int line, int scale)
{
    int winW;
    int winH;

    SDL_GetWindowSizeInPixels(window, &winW, &winH);

    const float pixW = 2.0f / winW;
    const float pixH = 2.0f / winH;
    const float x = -1.0f + (6 * pixW * indent);
    const float y = +1.0f - (6 * pixH * (line + 1));

    GLuint chars[16] = {0u};
    int i = 0;

    while (i < 16 && text[i] >= ASCII_MIN && text[i] <= ASCII_MAX) {
        chars[i] = BITMAPS[text[i] - ASCII_MIN];
        ++i;
    }

    glUniform2f(uniPos, x, y);
    glUniform2f(uniScale, pixW * scale, pixH * scale);
    glUniform1uiv(uniChars, i, chars);
    glDrawArrays(GL_TRIANGLES, 0, i * 6);
}


static void drawFrame()
{
    glClearColor(1.0f, 0.866f, 0.333f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);

    drawLabel("HELLO, WORLD!",     41, 22, 6);
    drawLabel("Bufferless text",   41, 35, 4);
    drawLabel("rendering demo:",   41, 43, 4);
    drawLabel(" !\"#$%&'()*+,-./", 41, 54, 2);
    drawLabel("0123456789:;<=>?",  41, 58, 2);
    drawLabel("@ABCDEFGHIJKLMNO",  41, 62, 2);
    drawLabel("PQRSTUVWXYZ[\\]^_", 41, 66, 2);
    drawLabel("`abcdefghijklmno",  41, 70, 2);
    drawLabel("pqrstuvwxyz{|}~",   41, 74, 2);
    drawLabel("The quick brown",   88, 54, 2);
    drawLabel("fox jumps over",    90, 58, 2);
    drawLabel("the lazy dog.",     92, 62, 2);
    drawLabel("OpenGL 3.3",        88, 74, 3);

    SDL_GL_SwapWindow(window);
}


static int pollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:    return 0;
            case SDL_KEYDOWN: return 0;
        }
    }
    return 1;
}


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (initWindow()) {
        drawFrame();
        while (pollEvents()) {}
    }
}
