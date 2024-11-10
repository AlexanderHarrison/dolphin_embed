#include "libretro.h"
#include "raylib.h"
#include "load.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#define CTX RETRO_HW_CONTEXT_D3D11

// GLOBALS ######################################################################

enum retro_pixel_format video_format = RETRO_PIXEL_FORMAT_UNKNOWN;


// CORE LOADING ############################################################################################
// taken from https://github.com/davidgfnet/miniretro

typedef uint64_t U64;
typedef int64_t I64;
typedef uint8_t U8;

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
           core_action_fnt                   retro_init;
           core_action_fnt                   retro_deinit;
           core_action_fnt                   retro_run;
           core_action_fnt                   retro_reset;
           core_info_fnt                     retro_get_info;
           core_loadg_fnt                    retro_load_game;
           core_unloadg_fnt                  retro_unload_game;
           core_set_environment_fnt          retro_set_environment_function;
           core_set_video_refresh_fnt        retro_set_video_refresh_function;
           core_set_audio_sample_fnt         retro_set_audio_sample_function;
           core_set_audio_sample_batch_fnt   retro_set_audio_sample_batch_function;
           core_set_input_poll_fnt           retro_set_input_poll_function;
           core_set_input_state_fnt          retro_set_input_state_function;
           core_serialize_fnt                retro_serialize;
           core_serialize_size_fnt           retro_serialize_size;
           core_unserialize_fnt              retro_unserialize;
           core_get_system_av_info_fnt       retro_get_system_av_info;

    void *handle;
} core_functions_t;

core_functions_t *load_core(const char *filename) {
    void *libhandle = lib_load(filename);
    if (!libhandle) {
        fprintf(stderr, "ERROR: failed to load library: %s\n", GetLastError());
        return NULL;
    }

    core_functions_t *fns = malloc(sizeof(core_functions_t));
    
    fns->retro_init                            = (core_action_fnt)                 lib_symbol(libhandle, "retro_init");
    fns->retro_deinit                          = (core_action_fnt)                 lib_symbol(libhandle, "retro_deinit");
    fns->retro_run                             = (core_action_fnt)                 lib_symbol(libhandle, "retro_run");
    fns->retro_reset                           = (core_action_fnt)                 lib_symbol(libhandle, "retro_reset");
    fns->retro_get_info                        = (core_info_fnt)                   lib_symbol(libhandle, "retro_get_system_info");
    fns->retro_load_game                       = (core_loadg_fnt)                  lib_symbol(libhandle, "retro_load_game");
    fns->retro_unload_game                     = (core_unloadg_fnt)                lib_symbol(libhandle, "retro_unload_game");

    fns->retro_set_environment_function        = (core_set_environment_fnt)        lib_symbol(libhandle, "retro_set_environment");
    fns->retro_set_video_refresh_function      = (core_set_video_refresh_fnt)      lib_symbol(libhandle, "retro_set_video_refresh");
    fns->retro_set_audio_sample_function       = (core_set_audio_sample_fnt)       lib_symbol(libhandle, "retro_set_audio_sample");
    fns->retro_set_audio_sample_batch_function = (core_set_audio_sample_batch_fnt) lib_symbol(libhandle, "retro_set_audio_sample_batch");
    fns->retro_set_input_poll_function         = (core_set_input_poll_fnt)         lib_symbol(libhandle, "retro_set_input_poll");
    fns->retro_set_input_state_function        = (core_set_input_state_fnt)        lib_symbol(libhandle, "retro_set_input_state");

    fns->retro_serialize                       = (core_serialize_fnt)              lib_symbol(libhandle, "retro_serialize");
    fns->retro_serialize_size                  = (core_serialize_size_fnt)         lib_symbol(libhandle, "retro_serialize_size");
    fns->retro_unserialize                     = (core_unserialize_fnt)            lib_symbol(libhandle, "retro_unserialize");
    fns->retro_get_system_av_info              = (core_get_system_av_info_fnt)     lib_symbol(libhandle, "retro_get_system_av_info");
    fns->handle = libhandle;

    return fns;
}

void unload_core(core_functions_t *core) {
    lib_unload(core->handle);
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
        return true;
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

bool tex_set = false;
Texture2D tex;

void RETRO_CALLCONV video_update(const void *data, unsigned width, unsigned height, size_t pitch) {
    printf("received frame w:%u h:%u pitch:%lu\n", width, height, pitch);

    if (data == NULL) return;
    Image image = {
        .data = data,
        .width = (int)width,
        .height = (int)height,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    Texture2D tex = LoadTextureFromImage(image);
    tex_set = true;
}

void RETRO_CALLCONV input_poll(void) { }
int16_t RETRO_CALLCONV input_state(unsigned port, unsigned device, unsigned index, unsigned id) {}

void RETRO_CALLCONV audio_sample(int16_t left, int16_t right) {}
size_t RETRO_CALLCONV audio_sample_batch(const int16_t *data, size_t frames) {}

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

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 480, "dolphin");
    SetTargetFPS(10);

#ifdef _WIN32
    const char *core_path = "C:\\Users\\Alex\\Documents\\Melee\\dolphin_embed\\libdolphin.dll";
    const char *iso_path =  "C:\\Users\\Alex\\Documents\\Melee\\melee vanilla.iso";
#else
    const char *core_path = "./libdolphin.so";
    const char *iso_path =  "/home/alex/melee/melee_vanilla.iso";
#endif

    printf("loading core '%s'\n", core_path);

    core_functions_t *core = load_core(core_path);
    assert(core != NULL);

    core->retro_set_environment_function(&env_callback);
    core->retro_set_video_refresh_function(&video_update);
    core->retro_set_audio_sample_function(audio_sample);
    core->retro_set_audio_sample_batch_function(audio_sample_batch);
    core->retro_set_input_poll_function(input_poll);
    core->retro_set_input_state_function(input_state);

    core->retro_init();

    Bytes iso = read_file(iso_path);
    assert(iso.ptr);

    struct retro_game_info gameinfo = {
        .path = iso_path,
        .data = iso.ptr, 
        .size = iso.size, 
        .meta = NULL
    };
    assert(core->retro_load_game(&gameinfo));
    core->retro_reset();

    struct retro_system_av_info avinfo;
    core->retro_get_system_av_info(&avinfo);

    core->retro_run();
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        if (tex_set)
            DrawTexture(tex, 0, 0, BLACK);
        EndDrawing();
        //printf("frame\n");
    }

    core->retro_unload_game();
    core->retro_deinit();
    unload_core(core);

    return 0;
}
