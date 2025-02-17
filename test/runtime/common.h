#include <cstddef>
#include <cstdint>

#include "HalideRuntime.h"
#include "msan_stubs.cpp"
#include "runtime_internal.h"
#include "to_string.cpp"

extern "C" {

extern int printf(const char *format, ...);

void halide_print(void *user_context, const char *str) {
    printf("%s", str);
}

void halide_error(void *user_context, const char *msg) {
    halide_print(user_context, msg);
}

void halide_profiler_report(void *user_context) {
}

void halide_profiler_reset() {
}

}  // extern "C"

#include "printer.h"

namespace {

size_t allocated_system_memory = 0;

void *align_up(void *ptr, size_t offset, size_t alignment) {
    return (void *)(((((size_t)ptr + offset)) + (alignment - 1)) & ~(alignment - 1));
}

void *allocate_system(void *user_context, size_t bytes) {
    constexpr size_t alignment = 128;
    constexpr size_t header_size = 2 * sizeof(size_t);
    size_t alloc_size = bytes + header_size + (alignment - 1);
    void *raw_ptr = malloc(alloc_size);
    if (raw_ptr == nullptr) {
        return nullptr;
    }
    void *aligned_ptr = align_up(raw_ptr, header_size, alignment);
    size_t aligned_offset = (size_t)((size_t)aligned_ptr - (size_t)raw_ptr);
    *((size_t *)aligned_ptr - 1) = aligned_offset;
    *((size_t *)aligned_ptr - 2) = alloc_size;
    allocated_system_memory += alloc_size;

    debug(user_context) << "Test : allocate_system ("
                        << "ptr=" << (void *)(raw_ptr) << " "
                        << "aligned_ptr=" << (void *)(aligned_ptr) << " "
                        << "aligned_offset=" << int32_t(aligned_offset) << " "
                        << "alloc_size=" << int32_t(alloc_size) << " "
                        << "allocated_system_memory=" << int32_t(allocated_system_memory) << " "
                        << ") !\n";

    return aligned_ptr;
}

void deallocate_system(void *user_context, void *aligned_ptr) {
    size_t aligned_offset = *((size_t *)aligned_ptr - 1);
    size_t alloc_size = *((size_t *)aligned_ptr - 2);
    void *raw_ptr = (void *)((uint8_t *)aligned_ptr - aligned_offset);
    free(raw_ptr);
    allocated_system_memory -= alloc_size;

    debug(user_context) << "Test : deallocate_system ("
                        << "ptr=" << (void *)(raw_ptr) << " "
                        << "aligned_ptr=" << (void *)(aligned_ptr) << " "
                        << "aligned_offset=" << int32_t(aligned_offset) << " "
                        << "alloc_size=" << int32_t(alloc_size) << " "
                        << "allocated_system_memory=" << int32_t(allocated_system_memory) << " "
                        << ") !\n";
}

}  // anonymous namespace
