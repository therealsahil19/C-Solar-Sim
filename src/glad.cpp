#include "glad.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <GL/glx.h>
#endif

// Function pointers - initialized to nullptr
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glad_glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glad_glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glad_glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = nullptr;
PFNGLCREATESHADERPROC glad_glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glad_glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glad_glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glad_glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glad_glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glad_glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glad_glUniform1i = nullptr;
PFNGLUNIFORM1FPROC glad_glUniform1f = nullptr;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = nullptr;
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = nullptr;

static void* loadProc(const char* name) {
#ifdef _WIN32
    // On Windows, use wglGetProcAddress for OpenGL extension functions
    void* proc = (void*)wglGetProcAddress(name);
    if (proc == nullptr || proc == (void*)0x1 || proc == (void*)0x2 || 
        proc == (void*)0x3 || proc == (void*)-1) {
        // Try loading from opengl32.dll directly for core functions
        HMODULE module = LoadLibraryA("opengl32.dll");
        if (module) {
            proc = (void*)GetProcAddress(module, name);
        }
    }
    return proc;
#else
    // On Linux/Unix, use glXGetProcAddress
    return (void*)glXGetProcAddressARB((const GLubyte*)name);
#endif
}

int gladLoadGL() {
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)loadProc("glGenVertexArrays");
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)loadProc("glBindVertexArray");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)loadProc("glDeleteVertexArrays");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC)loadProc("glGenBuffers");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC)loadProc("glBindBuffer");
    glad_glBufferData = (PFNGLBUFFERDATAPROC)loadProc("glBufferData");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)loadProc("glDeleteBuffers");
    glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)loadProc("glVertexAttribPointer");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)loadProc("glEnableVertexAttribArray");
    glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)loadProc("glDisableVertexAttribArray");
    glad_glCreateShader = (PFNGLCREATESHADERPROC)loadProc("glCreateShader");
    glad_glShaderSource = (PFNGLSHADERSOURCEPROC)loadProc("glShaderSource");
    glad_glCompileShader = (PFNGLCOMPILESHADERPROC)loadProc("glCompileShader");
    glad_glGetShaderiv = (PFNGLGETSHADERIVPROC)loadProc("glGetShaderiv");
    glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)loadProc("glGetShaderInfoLog");
    glad_glDeleteShader = (PFNGLDELETESHADERPROC)loadProc("glDeleteShader");
    glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC)loadProc("glCreateProgram");
    glad_glAttachShader = (PFNGLATTACHSHADERPROC)loadProc("glAttachShader");
    glad_glLinkProgram = (PFNGLLINKPROGRAMPROC)loadProc("glLinkProgram");
    glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)loadProc("glGetProgramiv");
    glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)loadProc("glGetProgramInfoLog");
    glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)loadProc("glDeleteProgram");
    glad_glUseProgram = (PFNGLUSEPROGRAMPROC)loadProc("glUseProgram");
    glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)loadProc("glGetUniformLocation");
    glad_glUniform1i = (PFNGLUNIFORM1IPROC)loadProc("glUniform1i");
    glad_glUniform1f = (PFNGLUNIFORM1FPROC)loadProc("glUniform1f");
    glad_glUniform3fv = (PFNGLUNIFORM3FVPROC)loadProc("glUniform3fv");
    glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)loadProc("glUniformMatrix3fv");
    glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)loadProc("glUniformMatrix4fv");
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC)loadProc("glActiveTexture");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)loadProc("glGenerateMipmap");
    glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)loadProc("glDrawElementsInstanced");
    glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)loadProc("glVertexAttribDivisor");
    
    // Return success if critical functions loaded
    return glad_glGenVertexArrays != nullptr && glad_glCreateShader != nullptr;
}
