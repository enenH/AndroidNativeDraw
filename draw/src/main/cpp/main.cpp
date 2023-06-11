#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>
#include <time.h>
#include "imgui_impl_android.h"
#include "TouchHelperA.h"
#include "Log.h"

#if defined(USE_OPENGL)
#include "imgui_impl_opengl3.h"
#include "OpenglUtils.h"

#else

#include "imgui_impl_vulkan.h"
#include "VulkanUtils.h"

#endif
//外部需要
float screenWidth = 0;
float screenHeight = 0;
uint32_t orientation = 0;
bool other_touch = false;

static JavaVM *g_jvm = nullptr;

static bool flag = true;

static bool Android_LoadSystemFont(float size) {
    char path[64]{0};
    char *filename = nullptr;
    const char *fontPath[] = {
            "/system/fonts", "/system/font", "/data/fonts"
    };
    for (auto tmp: fontPath) {
        if (access(tmp, R_OK) == 0) {
            strcpy(path, tmp);
            filename = path + strlen(tmp);
            break;
        }
    }
    if (!filename) {
        return false;
    }
    *filename++ = '/';
    strcpy(filename, "NotoSansCJK-Regular.ttc");
    if (access(path, R_OK) != 0) {
        strcpy(filename, "NotoSerifCJK-Regular.ttc");
        if (access(path, R_OK) != 0) {
            return false;
        }
    }
    ImGuiIO &io = ImGui::GetIO();
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        constexpr ImWchar Ranges[]{
                0x0020, 0x00FF, // Basic Latin
                0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
                0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
                0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
                0x0600, 0x06FF, // Arabic
                0x0E00, 0x0E7F, // Thai
                0x2DE0, 0x2DFF, // Cyrillic Extended-A
                0x2000, 0x206F, // General Punctuation
                0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0xFF00, 0xFFEF, // Half-width characters
                //0x4E00, 0x9FAF, // CJK Ideograms
                0xA640, 0xA69F, // Cyrillic Extended-B
                0x3131, 0x3163, // Korean alphabets
                //  0xAC00, 0xD7A3, // Korean characters
                0
        };
        builder.AddRanges(Ranges);
        builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.BuildRanges(&ranges);
    }
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.SizePixels = size;
    config.GlyphRanges = ranges.Data;
    config.OversampleH = 1;
    return io.Fonts->AddFontFromFileTTF(path, 0, &config);
}

const char *getClipText() {
    seteuid(2000);
    //获取env
    JNIEnv *env;
    g_jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    g_jvm->AttachCurrentThread(&env, NULL);

    jclass jniHelperClz = env->FindClass("com/example/draw/Main");
    jmethodID getClipText = env->GetStaticMethodID(jniHelperClz, "getClipText",
                                                   "()Ljava/lang/String;");
    jstring jstr = (jstring) env->CallStaticObjectMethod(jniHelperClz, getClipText);
    char *str = (char *) env->GetStringUTFChars(jstr, NULL);
    static char buff[1024];
    strncpy(buff, str, sizeof(buff) - 1);
    env->ReleaseStringUTFChars(jstr, str);
    //g_jvm->DetachCurrentThread();
    seteuid(0);
    return buff;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_draw_Main_native_1surfaceCreate(JNIEnv *env, jclass thiz, jobject surface,
                                                 jint width, jint height) {
    screenWidth = width;
    screenHeight = height;
    auto native_window = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_acquire(native_window);
#if defined(USE_OPENGL)
    SetupOpengl(native_window);
#else
    InitVulkan();
    SetupVulkan();
    SetupVulkanWindow(native_window, (int) screenWidth, (int) screenWidth);
#endif
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = {screenWidth, screenWidth};
    io.FontGlobalScale = 1.3;
    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(3);
    style.WindowRounding = 2;
    ImGui_ImplAndroid_Init(native_window);

#if defined(USE_OPENGL)
    ImGui_ImplOpenGL3_Init("#version 300 es");
#endif
    Android_LoadSystemFont(26);
#ifndef USE_OPENGL
    UploadFonts();
#endif
    //触摸初始化
    Touch_Init(screenHeight, screenWidth, true);
    while (flag) {
#ifdef USE_OPENGL
        ImGui_ImplOpenGL3_NewFrame();
#else
        ImGui_ImplVulkan_NewFrame();
#endif
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

#if defined(USE_OPENGL)
        const char* name = "OpenGL";
#else
        const char *name = "Vulkan";
#endif
        ImGui::SetNextWindowSize({500, 500}, ImGuiCond_Once);
        if (ImGui::Begin(name, &flag)) {
            ImGui::Text("Hello, world!");
            static const char *text = "null";
            ImGui::Text("%s", text);
            if (ImGui::Button("copy")) {
                text = getClipText();
            }
        }
        ImGui::End();
        // Rendering
        ImGui::Render();
#ifdef USE_OPENGL
        OpenglRender(ImGui::GetDrawData());
#else
        FrameRender(ImGui::GetDrawData());
        FramePresent();
#endif
    }
    Touch_Close();
    // Cleanup
#ifdef USE_OPENGL
    ImGui_ImplOpenGL3_Shutdown();
#else
    DeviceWait();
    ImGui_ImplVulkan_Shutdown();
#endif

    ImGui::DestroyContext();
#ifdef USE_OPENGL
    CleanupOpengl();
#else
    CleanupVulkanWindow();
    CleanupVulkan();
#endif
    ANativeWindow_release(native_window);
    LOGD("stop");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_draw_Main_native_1surfaceChanged(JNIEnv *env, jclass thiz, jint rotation) {
    orientation = rotation;
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnLoad");
    g_jvm = vm;
    return JNI_VERSION_1_6;
}
