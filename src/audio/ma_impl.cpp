#ifdef WITH_MINIAUDIO
    #define MINIAUDIO_IMPLEMENTATION
    #ifdef __APPLE__
        #define MA_NO_RUNTIME_LINKING
    #endif
    #include <miniaudio.h>
#endif
