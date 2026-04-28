#ifndef CRC32_RVV_HOOKS_H
#define CRC32_RVV_HOOKS_H

/**
 * CRC32 RVV HOOKS
 * RISC-V Vector Extension (RVV) hardware acceleration for CRC32
 */
ZLIB_INTERNAL extern unsigned long (*crc32_z_hook)(unsigned long crc, const unsigned char FAR *buf, z_size_t len);

#endif /* CRC32_RVV_HOOKS_H */
