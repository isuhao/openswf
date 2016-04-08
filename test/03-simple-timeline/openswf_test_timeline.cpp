#include "openswf_common.hpp"

extern "C" {
#include "GL/glew.h"
#include "GLFW/glfw3.h"
}

#define CHECK_GL_ERROR \
    do { \
        GLenum err = glGetError(); \
        if( err != GL_NO_ERROR ) { \
            printf("GL_%s - \n[%s :%d]\n", get_opengl_error(err), __FILE__, __LINE__); \
            assert(false); \
        } \
    } while(false);

static const char*
get_opengl_error(GLenum err)
{
    switch(err) {
        case GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        //case GL_INVALID_FRAMEBUFFER_OPERATION:
        //    error = "INVALID_FRAMEBUFFER_OPERATION";
        //    break;
    }
    return "";
}

using namespace openswf;

int frame = 0;

static void input_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        frame ++;
}

GLuint compile(GLenum type, const char* source)
{
    GLint status;
    
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    if (status == GL_FALSE) {
        char buf[1024];
        GLint len;
        glGetShaderInfoLog(shader, 1024, &len, buf);

        printf("compile failed:%s\n"
            "source:\n %s\n",
            buf, source);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint create_shader(const char* vs_src, const char* fs_src)
{
    auto prog = glCreateProgram();
    auto vs = compile(GL_VERTEX_SHADER, vs_src);
    if( vs == 0 ) 
        return 0;

    auto fs = compile(GL_FRAGMENT_SHADER, fs_src);
    if( fs == 0 )
        return 0;

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDetachShader(prog, fs);
    glDetachShader(prog, vs);
    glDeleteShader(fs);
    glDeleteShader(vs);

    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if( status == 0 )
    {
        char buf[1024];
        GLint len;
        glGetProgramInfoLog(prog, 1024, &len, buf);
        printf("link failed:%s\n", buf);
        return 0;
    }

    return prog;
}


int main(int argc, char* argv[])
{
    auto stream = create_from_file("../test/resources/simple-timeline-1.swf");
    auto player = Player::create(&stream);
    auto size   = player->get_size();
    
    if( !glfwInit() )
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(size.get_width(), size.get_height(), "03-Simple-Timeline", NULL, NULL);
    if( !window )
    {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, input_callback);
    glfwMakeContextCurrent(window);
    glfwSetTime(0);

    if( !Render::initilize() )
    {
        glfwTerminate();
        return -1;
    }

    CHECK_GL_ERROR
    
    auto& render = Render::get_instance();

    // The following is an 8x8 checkerboard pattern using // GL_RED, GL_UNSIGNED_BYTE data.
    static const GLubyte tex_checkerboard[] =
    {
        0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
        0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    };

    static float vertices[] = {
        -0.85f, -0.85f,
        0.85f, -0.85f,
        0.85f, 0.85f,
        -0.85f, 0.85f,

        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    static uint8_t indices[] = {
        0, 1, 2, 
        2, 3, 0,
    };

    static const char* vs =
        "#version 330 core\n"
        "layout(location = 0) in vec4 in_position;\n"
        "layout(location = 1) in vec2 in_tex_coord;\n"
        "out vec2 vs_tex_coord;"
        "void main()\n"
        "{\n"
        "  gl_Position = in_position;\n"
        "  vs_tex_coord = in_tex_coord;\n"
        "}\n";

    static const char* fs =
        "#version 330 core\n"
        "uniform sampler2D in_texture;\n"
        "in vec2 vs_tex_coord;\n"
        "out vec3 color;\n"
        "void main()\n"
        "{\n"
        "  color = texture(in_texture, vs_tex_coord).rgb;\n"
        "}\n";
    
    VertexAttribute attributes[2];
    attributes[0].vbslot = 0;
    attributes[0].n = 2;
    attributes[0].format = ElementFormat::FLOAT;

    attributes[1].vbslot = 0;
    attributes[1].n = 2,
    attributes[1].format = ElementFormat::FLOAT;
    attributes[1].offset = sizeof(float) * 4 * 2;

    const char* textures[] = { "in_texture" };

    auto vid = render.create_vertex_buffer(vertices, sizeof(vertices));
    auto iid = render.create_index_buffer(indices, sizeof(indices), ElementFormat::UNSIGNED_BYTE);
    auto tid = render.create_texture(tex_checkerboard, 8, 8, TextureFormat::ALPHA8, 0);
    auto pid = render.create_shader(vs, fs, 2, attributes, 1, textures);
    
    //auto last_time = glfwGetTime();

    while( !glfwWindowShouldClose(window) )
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        render.set_viewport(0, 0, width, height);
        render.clear(CLEAR_COLOR | CLEAR_DEPTH, 100, 100, 100, 255);

        // auto now_time = glfwGetTime();
        // player->update(now_time-last_time);
        // last_time = now_time;
        // player->render();

        render.bind(RenderObject::SHADER, pid);
        render.bind(RenderObject::VERTEX_BUFFER, vid, 0);
        render.bind(RenderObject::INDEX_BUFFER, iid, 0);
        render.bind(RenderObject::TEXTURE, tid, 0);
        render.draw(DrawMode::TRIANGLE, 0, 6);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete player;
    glfwTerminate();
    Render::dispose();
    return 0;
}