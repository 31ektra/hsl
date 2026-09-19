#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void hsl_kernel_panic(const char *message) __attribute__((noreturn));
void *hsl_kernel_alloc(size_t size, size_t alignment);
void hsl_kernel_free(void *pointer, size_t size, size_t alignment);
uint8_t hsl_mmio_read8(const volatile uint8_t *address);
uint16_t hsl_mmio_read16(const volatile uint16_t *address);
uint32_t hsl_mmio_read32(const volatile uint32_t *address);
uint64_t hsl_mmio_read64(const volatile uint64_t *address);
void hsl_mmio_write8(volatile uint8_t *address, uint8_t value);
void hsl_mmio_write16(volatile uint16_t *address, uint16_t value);
void hsl_mmio_write32(volatile uint32_t *address, uint32_t value);
void hsl_mmio_write64(volatile uint64_t *address, uint64_t value);
#ifdef __cplusplus
}
#endif
