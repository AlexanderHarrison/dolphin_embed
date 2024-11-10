#ifdef _WIN32
  #include <windows.h>
  #include <strsafe.h>
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

void *lib_load(const char *path) {
  return (void *)LOAD_LIBRARY(path);
}

void lib_unload(void *handle) {
  UNLOAD_LIBRARY(handle);
}

void *lib_symbol(void *handle, const char *name) {
  return LOAD_SYMBOL(handle, name);
}
