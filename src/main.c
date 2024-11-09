#include "libretro.h"

#include <raylib.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>

typedef uint64_t U64;
typedef int64_t I64;
typedef uint8_t U8;

// #define CTX RETRO_HW_CONTEXT_VULKAN
#define CTX RETRO_HW_CONTEXT_VULKAN

// GLOBALS ######################################################################

enum retro_pixel_format video_format = RETRO_PIXEL_FORMAT_UNKNOWN;

// CORE LOADING #################################################################
// taken from https://github.com/davidgfnet/miniretro

#ifdef WIN32
  #include <windows.h>
  #define LIBHANDLE HMODULE
  #define LOAD_LIBRARY(path) LoadLibraryA(path)
  #define UNLOAD_LIBRARY(lib) FreeLibrary(lib)
  #define LOAD_SYMBOL(lib, name) GetProcAddress(lib, name)
#else
  #include <dlfcn.h>
  #define LIBHANDLE void*
  #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY | RTLD_LOCAL)
  #define UNLOAD_LIBRARY(lib) dlclose(lib)
  #define LOAD_SYMBOL(lib, name) dlsym(lib, name)
#endif

typedef RETRO_CALLCONV void (*core_info_fnt)(struct retro_system_info *info);
typedef RETRO_CALLCONV void (*core_action_fnt)(void);
typedef RETRO_CALLCONV bool (*core_loadg_fnt)(const struct retro_game_info *game);
typedef RETRO_CALLCONV void (*core_unloadg_fnt)(void);
typedef RETRO_CALLCONV void (*core_set_environment_fnt)(retro_environment_t);
typedef RETRO_CALLCONV void (*core_set_video_refresh_fnt)(retro_video_refresh_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_fnt)(retro_audio_sample_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_batch_fnt)(retro_audio_sample_batch_t);
typedef RETRO_CALLCONV void (*core_set_input_poll_fnt)(retro_input_poll_t);
typedef RETRO_CALLCONV void (*core_set_input_state_fnt)(retro_input_state_t);
typedef RETRO_CALLCONV bool (*core_serialize_fnt)(void *data, size_t size);
typedef RETRO_CALLCONV size_t (*core_serialize_size_fnt)(void);
typedef RETRO_CALLCONV bool (*core_unserialize_fnt)(const void *data, size_t size);
typedef RETRO_CALLCONV void (*core_get_system_av_info_fnt)(struct retro_system_av_info *info);

typedef struct {
    core_action_fnt core_init;
    core_action_fnt core_deinit;
    core_action_fnt core_run;
    core_action_fnt core_reset;
    core_info_fnt   core_get_info;
    core_loadg_fnt  core_load_game;
    core_unloadg_fnt core_unload_game;
    core_set_environment_fnt core_set_env_function;
    core_set_video_refresh_fnt core_set_video_refresh_function;
    core_set_audio_sample_fnt core_set_audio_sample_function;
    core_set_audio_sample_batch_fnt core_set_audio_sample_batch_function;
    core_set_input_poll_fnt core_set_input_poll_function;
    core_set_input_state_fnt core_set_input_state_function;
    core_serialize_fnt core_serialize;
    core_serialize_size_fnt core_serialize_size;
    core_unserialize_fnt core_unserialize;
    core_get_system_av_info_fnt core_get_system_av_info;

    LIBHANDLE handle;
} core_functions_t;

core_functions_t *load_core(const char *filename) {
    LIBHANDLE libhandle = LOAD_LIBRARY(filename);
    if (!libhandle)
        return NULL;

    core_functions_t *fns = malloc(sizeof(core_functions_t));
    
    fns->core_init                            = (core_action_fnt)                 LOAD_SYMBOL(libhandle, "retro_init");
    fns->core_deinit                          = (core_action_fnt)                 LOAD_SYMBOL(libhandle, "retro_deinit");
    fns->core_run                             = (core_action_fnt)                 LOAD_SYMBOL(libhandle, "retro_run");
    fns->core_reset                           = (core_action_fnt)                 LOAD_SYMBOL(libhandle, "retro_reset");
    fns->core_get_info                        = (core_info_fnt)                   LOAD_SYMBOL(libhandle, "retro_get_system_info");
    fns->core_load_game                       = (core_loadg_fnt)                  LOAD_SYMBOL(libhandle, "retro_load_game");
    fns->core_unload_game                     = (core_unloadg_fnt)                LOAD_SYMBOL(libhandle, "retro_unload_game");

    fns->core_set_env_function                = (core_set_environment_fnt)        LOAD_SYMBOL(libhandle, "retro_set_environment");
    fns->core_set_video_refresh_function      = (core_set_video_refresh_fnt)      LOAD_SYMBOL(libhandle, "retro_set_video_refresh");
    fns->core_set_audio_sample_function       = (core_set_audio_sample_fnt)       LOAD_SYMBOL(libhandle, "retro_set_audio_sample");
    fns->core_set_audio_sample_batch_function = (core_set_audio_sample_batch_fnt) LOAD_SYMBOL(libhandle, "retro_set_audio_sample_batch");
    fns->core_set_input_poll_function         = (core_set_input_poll_fnt)         LOAD_SYMBOL(libhandle, "retro_set_input_poll");
    fns->core_set_input_state_function        = (core_set_input_state_fnt)        LOAD_SYMBOL(libhandle, "retro_set_input_state");

    fns->core_serialize                       = (core_serialize_fnt)              LOAD_SYMBOL(libhandle, "retro_serialize");
    fns->core_serialize_size                  = (core_serialize_size_fnt)         LOAD_SYMBOL(libhandle, "retro_serialize_size");
    fns->core_unserialize                     = (core_unserialize_fnt)            LOAD_SYMBOL(libhandle, "retro_unserialize");
    fns->core_get_system_av_info              = (core_get_system_av_info_fnt)     LOAD_SYMBOL(libhandle, "retro_get_system_av_info");
    fns->handle = libhandle;

    return fns;
}

void unload_core(core_functions_t *core) {
    UNLOAD_LIBRARY(core->handle);
    free(core);
}

void RETRO_CALLCONV logging_callback(enum retro_log_level level, const char *fmt, ...) {
    (void)level;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

bool RETRO_CALLCONV env_callback(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        printf("env set format %u\n", *(enum retro_pixel_format*)data);
        video_format = *(enum retro_pixel_format*)data;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE:
        return false;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *((const char**)data) = "/home/alex/melee/tutor/emu_embed";
        return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *((const char**)data) = "/home/alex/melee/tutor/emu_embed/data";
        return true;
    case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        return false;
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        return true;
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback*)data)->log = &logging_callback;
        return true;
    case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
        *((enum retro_hw_context_type *)data) = CTX;
        return false;
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
        if (data == NULL) return false;
        struct retro_hw_render_callback *i = data;
        if (i->context_type != CTX) return false;
        i->version_major = 1;
        i->version_minor = 0;
        i->context_reset = NULL;
        i->get_current_framebuffer = NULL;
        i->get_proc_address = NULL;
        i->depth = false;
        i->stencil = false;
        i->bottom_left_origin = false;
        i->cache_context = true;
        i->context_destroy = NULL;
        i->debug_context = NULL;
        return true;
    case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE:
        return false;
    default:
        printf("unhandled cmd %u\n", cmd);
        return false;
    }
}

void RETRO_CALLCONV video_update(const void *data, unsigned width, unsigned height, size_t pitch) {
    printf("video update\n");
    (void)pitch;

    //if (data == NULL) return;
    //Image image = {
    //    .data = data,
    //    .width = (int)width,
    //    .height = (int)height,
    //    .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    //    .mipmaps = 1
    //};
    //Texture2D tex = LoadTextureFromImage(image);

    BeginDrawing();
    ClearBackground(WHITE);
    //DrawTexture(tex, 0, 0, WHITE);
    EndDrawing();
}

// main ###########################################################################

typedef struct Bytes {
    U8 *ptr;
    U64 size;
} Bytes;

Bytes read_file(const char* path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return (Bytes) { .ptr = NULL, .size = 0 };
    fseek(f, 0, SEEK_END);
    I64 ret = ftell(f);
    fseek(f, 0, SEEK_SET);
    U64 fsize = (U64)ret;

    U8* c = malloc(fsize + 1);
    fread(c, fsize, 1, f);
    fclose(f);

    return (Bytes) {
        .ptr = c,
        .size = fsize,
    };
}

void alarmhandler(int signal) { (void)signal; exit(1); }

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 480, "dolphin");
    SetTargetFPS(60);

    core_functions_t *core = load_core("./libdolphin.so");
    assert(core != NULL);

    core->core_set_env_function(&env_callback);
    core->core_set_video_refresh_function(&video_update);
    core->core_set_audio_sample_function(NULL);
    core->core_set_audio_sample_batch_function(NULL);
    core->core_set_input_poll_function(NULL);
    core->core_set_input_state_function(NULL);

    core->core_init();

    const char *path = "/home/alex/melee/melee_vanilla.iso";
    Bytes iso = read_file(path);
    assert(iso.ptr);

    struct retro_game_info gameinfo = {
        .path = path,
        .data = iso.ptr, 
        .size = iso.size, 
        .meta = NULL
    };
    assert(core->core_load_game(&gameinfo));

    while (!WindowShouldClose()) {
        core->core_run();
        printf("frame\n");
    }

    //core->core_unload_game();
    //core->core_deinit();
    //unload_core(core);

    //CloseWindow();

    return 0;
}
