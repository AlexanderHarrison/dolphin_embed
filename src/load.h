void *lib_load(const char *path);
void lib_unload(void *handle);
void *lib_symbol(void *handle, const char *name);
