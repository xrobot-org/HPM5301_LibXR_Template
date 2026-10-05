# HPM SDK skips compiler probes; LibXR needs the real C++ feature information.
# Keep probes link-independent for the bare-metal target.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER_FORCED FALSE)
set(CMAKE_CXX_COMPILER_FORCED FALSE)
