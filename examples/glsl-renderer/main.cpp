// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <formula/parser/ParseOptions.h>
#include <formula/parser/Parser.h>
#include <formula/translator/GLSLEmitter.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

constexpr int IMAGE_WIDTH{16};
constexpr int IMAGE_HEIGHT{16};

const char *DEFAULT_FORMULA{"init:\n"
                            "  z = pixel\n"
                            "loop:\n"
                            "  z = sqr(z) + p1\n"
                            "bailout:\n"
                            "  |z| < 4\n"};

struct GlfwSession
{
    GlfwSession()
    {
        if (glfwInit() != GLFW_TRUE)
        {
            throw std::runtime_error{"failed to initialize GLFW"};
        }
    }

    ~GlfwSession()
    {
        glfwTerminate();
    }

    GlfwSession(const GlfwSession &) = delete;
    GlfwSession(GlfwSession &&) = delete;
    GlfwSession &operator=(const GlfwSession &) = delete;
    GlfwSession &operator=(GlfwSession &&) = delete;
};

struct GlfwWindow
{
    GlfwWindow()
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(IMAGE_WIDTH, IMAGE_HEIGHT, "GLSL Renderer", nullptr, nullptr);
        if (window == nullptr)
        {
            throw std::runtime_error{"failed to create an OpenGL 4.5 context"};
        }
        glfwMakeContextCurrent(window);
    }

    ~GlfwWindow()
    {
        if (window != nullptr)
        {
            glfwDestroyWindow(window);
        }
    }

    GlfwWindow(const GlfwWindow &) = delete;
    GlfwWindow(GlfwWindow &&) = delete;
    GlfwWindow &operator=(const GlfwWindow &) = delete;
    GlfwWindow &operator=(GlfwWindow &&) = delete;

    GLFWwindow *window{};
};

struct ShaderObject
{
    ShaderObject() = default;
    explicit ShaderObject(GLuint handle_) :
        handle(handle_)
    {
    }

    ~ShaderObject()
    {
        if (handle != 0U)
        {
            glDeleteShader(handle);
        }
    }

    ShaderObject(const ShaderObject &) = delete;
    ShaderObject(ShaderObject &&rhs) noexcept :
        handle(std::exchange(rhs.handle, 0U))
    {
    }
    ShaderObject &operator=(const ShaderObject &) = delete;
    ShaderObject &operator=(ShaderObject &&rhs) noexcept
    {
        if (this != &rhs)
        {
            if (handle != 0U)
            {
                glDeleteShader(handle);
            }
            handle = std::exchange(rhs.handle, 0U);
        }
        return *this;
    }

    GLuint handle{};
};

struct ProgramObject
{
    ProgramObject() = default;
    explicit ProgramObject(GLuint handle_) :
        handle(handle_)
    {
    }

    ~ProgramObject()
    {
        if (handle != 0U)
        {
            glDeleteProgram(handle);
        }
    }

    ProgramObject(const ProgramObject &) = delete;
    ProgramObject(ProgramObject &&rhs) noexcept :
        handle(std::exchange(rhs.handle, 0U))
    {
    }
    ProgramObject &operator=(const ProgramObject &) = delete;
    ProgramObject &operator=(ProgramObject &&rhs) noexcept
    {
        if (this != &rhs)
        {
            if (handle != 0U)
            {
                glDeleteProgram(handle);
            }
            handle = std::exchange(rhs.handle, 0U);
        }
        return *this;
    }

    GLuint handle{};
};

struct TextureObject
{
    TextureObject() = default;

    ~TextureObject()
    {
        if (handle != 0U)
        {
            glDeleteTextures(1, &handle);
        }
    }

    TextureObject(const TextureObject &) = delete;
    TextureObject(TextureObject &&rhs) noexcept :
        handle(std::exchange(rhs.handle, 0U))
    {
    }
    TextureObject &operator=(const TextureObject &) = delete;
    TextureObject &operator=(TextureObject &&rhs) noexcept
    {
        if (this != &rhs)
        {
            if (handle != 0U)
            {
                glDeleteTextures(1, &handle);
            }
            handle = std::exchange(rhs.handle, 0U);
        }
        return *this;
    }

    GLuint handle{};
};

struct BufferObject
{
    BufferObject() = default;

    ~BufferObject()
    {
        if (handle != 0U)
        {
            glDeleteBuffers(1, &handle);
        }
    }

    BufferObject(const BufferObject &) = delete;
    BufferObject(BufferObject &&rhs) noexcept :
        handle(std::exchange(rhs.handle, 0U))
    {
    }
    BufferObject &operator=(const BufferObject &) = delete;
    BufferObject &operator=(BufferObject &&rhs) noexcept
    {
        if (this != &rhs)
        {
            if (handle != 0U)
            {
                glDeleteBuffers(1, &handle);
            }
            handle = std::exchange(rhs.handle, 0U);
        }
        return *this;
    }

    GLuint handle{};
};

std::string shader_info_log(GLuint shader)
{
    GLint length{};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length), '\0');
    if (length > 0)
    {
        glGetShaderInfoLog(shader, length, nullptr, log.data());
    }
    return log;
}

std::string program_info_log(GLuint program)
{
    GLint length{};
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length), '\0');
    if (length > 0)
    {
        glGetProgramInfoLog(program, length, nullptr, log.data());
    }
    return log;
}

ShaderObject compile_compute_shader(std::string_view source)
{
    ShaderObject shader{glCreateShader(GL_COMPUTE_SHADER)};
    const char *text{source.data()};
    const GLint size{static_cast<GLint>(source.size())};
    glShaderSource(shader.handle, 1, &text, &size);
    glCompileShader(shader.handle);

    GLint status{};
    glGetShaderiv(shader.handle, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        throw std::runtime_error{"shader compile failed:\n" + shader_info_log(shader.handle)};
    }
    return shader;
}

ProgramObject link_program(GLuint shader)
{
    ProgramObject program{glCreateProgram()};
    glAttachShader(program.handle, shader);
    glLinkProgram(program.handle);

    GLint status{};
    glGetProgramiv(program.handle, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        throw std::runtime_error{"program link failed:\n" + program_info_log(program.handle)};
    }
    return program;
}

TextureObject create_output_texture()
{
    TextureObject texture;
    glGenTextures(1, &texture.handle);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, IMAGE_WIDTH, IMAGE_HEIGHT);
    glBindImageTexture(0, texture.handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    return texture;
}

BufferObject create_parameter_buffer()
{
    struct Parameters
    {
        std::array<float, 2> p1{0.0F, 0.0F};
        std::array<float, 2> p2{0.0F, 0.0F};
        std::array<float, 2> p3{0.0F, 0.0F};
        std::array<float, 2> p4{0.0F, 0.0F};
        std::array<float, 2> p5{0.0F, 0.0F};
        std::array<float, 2> center{0.0F, 0.0F};
        std::array<float, 2> view_size{3.0F, 3.0F};
        std::array<std::uint32_t, 2> resolution{IMAGE_WIDTH, IMAGE_HEIGHT};
        std::uint32_t maxit{64U};
        float bailout{4.0F};
    };

    const Parameters parameters;
    BufferObject buffer;
    glGenBuffers(1, &buffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(parameters), &parameters, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, buffer.handle);
    return buffer;
}

std::string emit_default_shader()
{
    formula::parser::Options options;
    const formula::parser::ParserPtr parser{formula::parser::create_parser(DEFAULT_FORMULA, options)};
    const formula::ast::FormulaSectionsPtr ast{parser->parse()};
    if (!parser->get_errors().empty() || !ast)
    {
        std::string message{"failed to parse default formula"};
        for (const formula::parser::Diagnostic &diagnostic : parser->get_errors())
        {
            message += "\n";
            message += formula::parser::to_string(diagnostic.code);
        }
        throw std::runtime_error{message};
    }
    return formula::codegen::emit_shader(*ast);
}

void render_shader(const std::string &shader_source)
{
    const GlfwSession session;
    const GlfwWindow window;
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        throw std::runtime_error{"failed to load OpenGL entry points"};
    }

    const ShaderObject shader{compile_compute_shader(shader_source)};
    const ProgramObject program{link_program(shader.handle)};
    const TextureObject texture{create_output_texture()};
    const BufferObject parameters{create_parameter_buffer()};

    glUseProgram(program.handle);
    glDispatchCompute((IMAGE_WIDTH + 7U) / 8U, (IMAGE_HEIGHT + 7U) / 8U, 1U);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<float> pixels(static_cast<std::size_t>(IMAGE_WIDTH * IMAGE_HEIGHT * 4));
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    if (glGetError() != GL_NO_ERROR)
    {
        throw std::runtime_error{"OpenGL render failed"};
    }

    (void) parameters;
    std::cout << "Rendered " << IMAGE_WIDTH << "x" << IMAGE_HEIGHT << " GLSL smoke image.\n";
    std::cout << "First pixel: " << pixels[0] << ", " << pixels[1] << ", " << pixels[2] << ", " << pixels[3] << "\n";
}

} // namespace

int main()
{
    try
    {
        render_shader(emit_default_shader());
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
