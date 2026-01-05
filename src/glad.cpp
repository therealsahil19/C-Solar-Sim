// GLAD OpenGL Loader Implementation
#include "glad.h"

#ifdef _WIN32
#include <windows.h>

// Function pointers
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = nullptr;

static void* loadProc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p == nullptr || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 || p == (void*)-1) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p = (void*)GetProcAddress(module, name);
    }
    return p;
}

int gladLoadGL() {
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)loadProc("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)loadProc("glBindVertexArray");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)loadProc("glDeleteVertexArrays");
    glGenBuffers = (PFNGLGENBUFFERSPROC)loadProc("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)loadProc("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)loadProc("glBufferData");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)loadProc("glDeleteBuffers");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)loadProc("glVertexAttribPointer");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)loadProc("glEnableVertexAttribArray");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)loadProc("glDisableVertexAttribArray");
    glCreateShader = (PFNGLCREATESHADERPROC)loadProc("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)loadProc("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)loadProc("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)loadProc("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)loadProc("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)loadProc("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)loadProc("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)loadProc("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)loadProc("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)loadProc("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)loadProc("glGetProgramInfoLog");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)loadProc("glDeleteProgram");
    glUseProgram = (PFNGLUSEPROGRAMPROC)loadProc("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)loadProc("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)loadProc("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)loadProc("glUniform1f");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)loadProc("glUniform3fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)loadProc("glUniformMatrix3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)loadProc("glUniformMatrix4fv");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)loadProc("glActiveTexture");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)loadProc("glGenerateMipmap");
    
    return glGenVertexArrays != nullptr && glCreateShader != nullptr;
}

#else
// Linux/Mac implementation would go here
int gladLoadGL() { return 0; }
#endif
