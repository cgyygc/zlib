# Check if we compile for RISC-V
#
CHECK_C_SOURCE_COMPILES("
#if !defined(__riscv) && !defined(__riscv__)
    #error
#endif
int main() {return 0;}
" HAS_RISCV_SUPPORT)

#
# Check for RISC-V RVV (Vector Extension)
#
if(ZLIB_WITH_CRC32RVV AND HAS_RISCV_SUPPORT)
    # Preset the compiler specific flags
    # Note: -march flag may need adjustment based on the target RISC-V processor
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(RVVFLAG "-march=rv64gcv")
    else()
        # GCC uses similar flags
        set(RVVFLAG "-march=rv64gcv")
    endif(CMAKE_C_COMPILER_ID STREQUAL "Clang")

    # Test for RVV support
    set(RISCV_RVV_TEST
        "#if !defined(__riscv) && !defined(__riscv__) \n\
        #error \n\
        #endif \n\
        #include <riscv_vector.h> \n\
        int main(void) { \
           vuint8m1_t a = __riscv_vle8_v_u8m1((const uint8_t*)0, 1); \
           return 0; \
        }")

    # Check if compiler supports RVV with current flags
    set(CMAKE_REQUIRED_FLAGS "${RVVFLAG}")
    check_c_source_compiles("${RISCV_RVV_TEST}" HAS_RISCV_RVV_SUPPORT)

    # If not supported, try with Zbc extension (carry-less multiply)
    if(NOT HAS_RISCV_RVV_SUPPORT)
        set(RVVFLAG "-march=rv64gcv_zbc")
        check_c_source_compiles("${RISCV_RVV_TEST}" HAS_RISCV_RVV_ZBC_SUPPORT)
        if(HAS_RISCV_RVV_ZBC_SUPPORT)
            set(HAS_RISCV_RVV_SUPPORT 1)
        endif()
    endif()

    unset(CMAKE_REQUIRED_FLAGS)

    # Prepare compiling for RISC-V with RVV
    if(HAS_RISCV_RVV_SUPPORT)
        if(ZLIB_BUILD_SHARED)
            target_sources(zlib
                PRIVATE
                    crc32_rvv.c
                    crc32_rvv_hooks.h)
            target_compile_definitions(zlib PUBLIC -DHAVE_RISCV_RVV=1)
        endif(ZLIB_BUILD_SHARED)
        if(ZLIB_BUILD_STATIC)
            target_sources(zlibstatic
                PRIVATE
                    crc32_rvv.c
                    crc32_rvv_hooks.h)
            target_compile_definitions(zlibstatic PUBLIC -DHAVE_RISCV_RVV=1)
        endif(ZLIB_BUILD_STATIC)
        set_source_files_properties(
            crc32_rvv.c
            PROPERTIES COMPILE_OPTIONS "${RVVFLAG}")
    endif(HAS_RISCV_RVV_SUPPORT)
endif(ZLIB_WITH_CRC32RVV AND HAS_RISCV_SUPPORT)
