#include "girassol/world.hpp"

#include <jni.h>
#include <memory>
#include <mutex>
#include <string>

namespace {
std::mutex g_mutex;
std::unique_ptr<girassol::WorldEngine> g_world;

jstring to_java(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}
void throw_java(JNIEnv* env, const char* class_name, const char* message) noexcept {
    if (env->ExceptionCheck()) return;
    jclass cls = env->FindClass(class_name);
    if (cls != nullptr) {
        env->ThrowNew(cls, message);
        env->DeleteLocalRef(cls);
    }
}
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_girassol_app_NativeWorld_nativeSnapshot(JNIEnv* env, jobject) noexcept {
    try {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_world) g_world = std::make_unique<girassol::WorldEngine>();
        return to_java(env, g_world->snapshot_json());
    } catch (const std::exception& e) {
        throw_java(env, "java/lang/IllegalStateException", e.what());
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "native snapshot failed");
    }
    return nullptr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_girassol_app_NativeWorld_nativeAdvance(JNIEnv* env, jobject, jdouble hours) noexcept {
    try {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_world) g_world = std::make_unique<girassol::WorldEngine>();
        g_world->step(static_cast<double>(hours));
        return to_java(env, g_world->snapshot_json());
    } catch (const std::invalid_argument& e) {
        throw_java(env, "java/lang/IllegalArgumentException", e.what());
    } catch (const std::exception& e) {
        throw_java(env, "java/lang/IllegalStateException", e.what());
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "native advance failed");
    }
    return nullptr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_girassol_app_NativeWorld_nativeDialogue(JNIEnv* env, jobject, jlong citizen_id) noexcept {
    try {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_world) g_world = std::make_unique<girassol::WorldEngine>();
        return to_java(env, girassol::WorldEngine::local_dialogue(g_world->state(), static_cast<std::uint64_t>(citizen_id)));
    } catch (...) {
        throw_java(env, "java/lang/IllegalStateException", "native dialogue failed");
        return nullptr;
    }
}
