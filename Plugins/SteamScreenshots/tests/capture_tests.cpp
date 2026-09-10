#include <glew.h>
#include <GLFW/glfw3.h>
#include <metahook.h>
#include "../gl_capture.h"

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
    constexpr int CanvasWidth = 1400;
    constexpr int CanvasHeight = 128;
    constexpr int CaptureHeight = 7;
    constexpr int RGBChannels = 3;
    constexpr auto CaptureTimeout = std::chrono::seconds(5);

    int captureWidth = 1366;
    int captureHeight = CaptureHeight;
    int callbackCount = 0;
    int logCount = 0;
    int receivedWidth = 0;
    int receivedHeight = 0;
    std::vector<byte> receivedPixels;

    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void CheckGLError()
    {
        const auto error = glGetError();
        if (error != GL_NO_ERROR)
            throw std::runtime_error("OpenGL error: " + std::to_string(error));
    }

    void ReceiveCapture(void* pixels, size_t size, int width, int height)
    {
        ++callbackCount;
        receivedWidth = width;
        receivedHeight = height;
        const auto begin = static_cast<const byte*>(pixels);
        receivedPixels.assign(begin, begin + size);
    }

    struct GLState
    {
        GLint readFBO, drawFBO, pbo, alignment, rowLength, skipRows, skipPixels;
        bool operator==(const GLState&) const = default;

        static GLState Read()
        {
            GLState state{};
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &state.readFBO);
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state.drawFBO);
            glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &state.pbo);
            glGetIntegerv(GL_PACK_ALIGNMENT, &state.alignment);
            glGetIntegerv(GL_PACK_ROW_LENGTH, &state.rowLength);
            glGetIntegerv(GL_PACK_SKIP_ROWS, &state.skipRows);
            glGetIntegerv(GL_PACK_SKIP_PIXELS, &state.skipPixels);
            return state;
        }
    };

    struct Context
    {
        GLFWwindow* window = nullptr;
        GLuint callerPBO = 0;
        std::array<GLuint, 2> callerFBOs{};

        Context()
        {
            glfwSetErrorCallback([](int error, const char* message) {
                std::fprintf(stderr, "GLFW %d: %s\n", error, message);
            });
            Require(glfwInit() == GLFW_TRUE, "GLFW initialization failed");
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            window = glfwCreateWindow(CanvasWidth, CanvasHeight, "Capture tests", nullptr, nullptr);
            if (!window)
            {
                glfwTerminate();
                throw std::runtime_error("OpenGL 3.3 context unavailable");
            }
            glfwMakeContextCurrent(window);
            glewExperimental = GL_TRUE;
            const auto result = glewInit();
            if (result != GLEW_OK)
            {
                glfwDestroyWindow(window);
                glfwTerminate();
                throw std::runtime_error("GLEW initialization failed");
            }
            // Some GLEW versions probe GL_EXTENSIONS in a core context.
            while (glGetError() != GL_NO_ERROR) {}
            std::printf("OpenGL: %s / %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
        }

        ~Context()
        {
            GL_ShutdownCapture();
            glDeleteBuffers(1, &callerPBO);
            glDeleteFramebuffers(static_cast<GLsizei>(callerFBOs.size()), callerFBOs.data());
            glfwDestroyWindow(window);
            glfwTerminate();
        }

        void SetCallerState(bool dirtyPacking)
        {
            glGenBuffers(1, &callerPBO);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, callerPBO);
            constexpr GLsizeiptr CallerBufferSize = 64;
            glBufferData(GL_PIXEL_PACK_BUFFER, CallerBufferSize, nullptr, GL_STREAM_READ);
            glGenFramebuffers(static_cast<GLsizei>(callerFBOs.size()), callerFBOs.data());
            glBindFramebuffer(GL_READ_FRAMEBUFFER, callerFBOs[0]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, callerFBOs[1]);
            if (dirtyPacking)
            {
                glPixelStorei(GL_PACK_ALIGNMENT, 8);
                glPixelStorei(GL_PACK_ROW_LENGTH, CanvasWidth + 10);
                glPixelStorei(GL_PACK_SKIP_ROWS, 2);
                glPixelStorei(GL_PACK_SKIP_PIXELS, 3);
            }
        }

        void Paint()
        {
            int framebufferWidth = 0, framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            Require(framebufferWidth >= captureWidth && framebufferHeight >= captureHeight,
                "Hidden window framebuffer is too small for the capture region");
            const auto original = GLState::Read();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glDrawBuffer(GL_BACK);
            glDisable(GL_DITHER);
            glEnable(GL_SCISSOR_TEST);
            for (int y = 0; y < captureHeight; ++y)
            {
                glScissor(0, y, captureWidth, 1);
                glClearColor(y % 2 == 0 ? 1.0f : 0.0f, y % 2 == 0 ? 0.0f : 1.0f, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            // A vertical blue edge catches horizontal offsets and row-stride errors.
            glScissor(captureWidth - 1, 0, 1, captureHeight);
            glClearColor(0, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original.drawFBO);
            CheckGLError();
        }
    };

    void AwaitCapture(int expectedCount)
    {
        const auto deadline = std::chrono::steady_clock::now() + CaptureTimeout;
        while (callbackCount < expectedCount && std::chrono::steady_clock::now() < deadline)
        {
            GL_QueryAsyncCapture(ReceiveCapture);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        Require(callbackCount == expectedCount, "Screenshot callback count or completion timeout");
    }

    void CheckPixels()
    {
        Require(receivedWidth == captureWidth && receivedHeight == captureHeight, "Incorrect screenshot dimensions");
        Require(receivedPixels.size() == size_t(captureWidth * captureHeight * RGBChannels), "Incorrect RGB byte count");
        for (int y = 0; y < captureHeight; ++y)
        {
            const int sourceY = captureHeight - y - 1;
            for (int x = 0; x < captureWidth; ++x)
            {
                const bool edge = x == captureWidth - 1;
                const std::array<byte, RGBChannels> expected{
                    byte(!edge && sourceY % 2 == 0 ? 255 : 0),
                    byte(!edge && sourceY % 2 != 0 ? 255 : 0),
                    byte(edge ? 255 : 0)
                };
                for (int channel = 0; channel < RGBChannels; ++channel)
                    Require(expected[channel] == receivedPixels[(y * captureWidth + x) * RGBChannels + channel],
                        "RGB content, row stride, or vertical orientation mismatch");
            }
        }
    }

    GLenum APIENTRY FailedWait(GLsync, GLbitfield, GLuint64) { return GL_WAIT_FAILED; }
    GLenum APIENTRY TimedOutWait(GLsync, GLbitfield, GLuint64) { return GL_TIMEOUT_EXPIRED; }

    void TestTimeout(Context& context)
    {
        context.SetCallerState(true);
        Require(GL_InitCapture(), "Capture initialization failed");
        const auto original = GLState::Read();
        context.Paint();
        GL_BeginCapture(ReceiveCapture);
        const auto realWait = glClientWaitSync;
        glClientWaitSync = TimedOutWait;
        GL_QueryAsyncCapture(ReceiveCapture);
        glClientWaitSync = realWait;
        Require(callbackCount == 0, "Timeout delivered an unfinished screenshot");
        // Resume the existing request without issuing a replacement capture.
        AwaitCapture(1);
        Require(original == GLState::Read(), "Timeout/resume changed caller GL state");
        CheckGLError();
        CheckPixels();
    }

    void TestCapture(Context& context, bool dirtyPacking)
    {
        context.SetCallerState(dirtyPacking);
        const auto original = GLState::Read();
        Require(GL_InitCapture(), "Capture initialization failed");
        Require(original == GLState::Read(), "Initialization changed caller GL state");

        // Exercise every RGB row remainder, including both growth and shrinkage.
        for (const int width : {1366, 1367, 1365, 1364})
        {
            captureWidth = width;
            captureHeight = width == 1367 ? CaptureHeight + 1 : CaptureHeight;
            context.Paint();
            const int expectedCount = callbackCount + 1;
            GL_BeginCapture(ReceiveCapture);
            Require(original == GLState::Read(), "Begin/resize changed caller GL state");
            AwaitCapture(expectedCount);
            Require(original == GLState::Read(), "Async query changed caller GL state");
            CheckGLError();
            CheckPixels();
        }
    }

    void TestFailedWait(Context& context)
    {
        context.SetCallerState(true);
        Require(GL_InitCapture(), "Capture initialization failed");
        const auto original = GLState::Read();
        context.Paint();
        GL_BeginCapture(ReceiveCapture);
        const auto realWait = glClientWaitSync;
        // Inject only the wait status; pixels, PBOs, mappings and fences remain real GL.
        glClientWaitSync = TimedOutWait;
        GL_QueryAsyncCapture(ReceiveCapture);
        const bool timeoutKeptPending = callbackCount == 0;
        glClientWaitSync = FailedWait;
        GL_QueryAsyncCapture(ReceiveCapture);
        glClientWaitSync = realWait;
        Require(timeoutKeptPending, "Timeout delivered an unfinished screenshot");
        Require(callbackCount == 0 && logCount == 1, "Wait failure was not discarded and reported");
        // A different size distinguishes a new capture from the failed request.
        captureWidth = 1367;
        context.Paint();
        GL_BeginCapture(ReceiveCapture);
        AwaitCapture(1);
        Require(original == GLState::Read(), "Failure/recovery changed caller GL state");
        CheckGLError();
        CheckPixels();
    }
}

CaptureTestVideoAPI videoAPI;
CaptureTestVideoAPI* g_pMetaHookAPI = &videoAPI;
CaptureTestEngine gEngfuncs;

void CaptureTestVideoAPI::GetVideoMode(int* width, int* height, void*, void*)
{
    *width = captureWidth;
    *height = captureHeight;
}

void CaptureTestEngine::Con_Printf(const char* format, ...)
{
    ++logCount;
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "Expected a CTest case name");
        const std::string testCase = argv[1];
        Context context;
        if (testCase == "framebuffer-unavailable" || testCase == "framebuffer-entrypoint-unavailable")
        {
            const auto original = GLState::Read();
            const auto core = GLEW_VERSION_3_0;
            const auto arb = GLEW_ARB_framebuffer_object;
            const auto bind = glBindFramebuffer;
            if (testCase == "framebuffer-unavailable")
            {
                __GLEW_VERSION_3_0 = GL_FALSE;
                __GLEW_ARB_framebuffer_object = GL_FALSE;
            }
            else
                glBindFramebuffer = nullptr;
            const bool initialized = GL_InitCapture();
            __GLEW_VERSION_3_0 = core;
            __GLEW_ARB_framebuffer_object = arb;
            glBindFramebuffer = bind;
            Require(!initialized, "Missing framebuffer capability must reject capture initialization");
            Require(original == GLState::Read(), "Rejected initialization changed GL state");
            CheckGLError();
        }
        else if (testCase == "wait-failure")
            TestFailedWait(context);
        else if (testCase == "wait-timeout")
            TestTimeout(context);
        else
        {
            Require(testCase == "sync-default" || testCase == "sync-dirty" || testCase == "async-default"
                || testCase == "async-dirty" || testCase == "arb-framebuffer", "Unknown test case");
            if (testCase.starts_with("sync") || testCase == "arb-framebuffer")
                __GLEW_VERSION_3_2 = GL_FALSE;
            if (testCase == "arb-framebuffer")
            {
                __GLEW_VERSION_3_0 = GL_FALSE;
                __GLEW_ARB_framebuffer_object = GL_TRUE;
            }
            TestCapture(context, testCase.ends_with("dirty"));
        }
        std::printf("PASS: %s\n", testCase.c_str());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
