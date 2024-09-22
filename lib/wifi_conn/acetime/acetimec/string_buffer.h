/*
 * MIT License
 * Copyright (c) 2022 Brian T. Park
 */

/**
 * @file string_buffer.h
 *
 * A simple set of integer to string conversion functions.
 */

#ifndef ACE_TIME_C_STRING_BUFFER_H
#define ACE_TIME_C_STRING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** An array of characters that knows its capacity and its current usage size */
typedef struct AtcStringBuffer {
  /** Pointer to base of the string buffer. */
  char *p;
  /** Total allocated size of buffer. */
  uint8_t capacity;
  /** Current buffer usage. */
  uint8_t size;
} AtcStringBuffer;

/** Initialize the AtcStringBuffer with its char pointer and capacity. */
void atc_buf_init(AtcStringBuffer *sb, char *p, uint8_t capacity);

/** Reset the buffer to zero length. */
void atc_buf_reset(AtcStringBuffer *sb);

/** Finalize the AtcStringBuffer by terminating the string with a NUL. */
void atc_buf_close(AtcStringBuffer *sb);

/** Print one char into the AtcStringBuffer. */
void atc_print_char(AtcStringBuffer *sb, char c);

/** Print the given string into the AtcStringBuffer. */
void atc_print_string(AtcStringBuffer *sb, const char *s);

/** Print the human-readable digits of uint16_t into the AtcStringBuffer. */
void atc_print_uint16(AtcStringBuffer *sb, uint16_t n);

/** Print the unsigned integer padded to 2 digits. 1 is printed as "01". */
void atc_print_uint16_pad2(AtcStringBuffer *sb, uint16_t n);

/** Print the unsigned integer padded to 4 digits. 12 is printed as "0012". */
void atc_print_uint16_pad4(AtcStringBuffer *sb, uint16_t n);

/** Print the signed integer. A negative number will print a leading "-". */
void atc_print_int16(AtcStringBuffer *sb, int16_t n);

#ifdef __cplusplus
}
#endif

#endif
