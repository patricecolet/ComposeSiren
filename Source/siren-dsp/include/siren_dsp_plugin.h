/* The siren-dsp C ABI the plugin uses with COMPOSESIREN_RUST_DSP (src/plugin.rs):
 * one Rust synthesis per siren voice, driven like the C++ Sirene. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsrSiren CsrSiren;

/* Loads (or shares) the data: not on the audio thread. Null on error. */
CsrSiren* csr_siren_new(const char* resources, const char* siren, const char* variant, const char* dataset);
void csr_siren_free(CsrSiren* s);
void csr_siren_set_sample_rate(CsrSiren* s, double sample_rate);
void csr_siren_set_velocite(CsrSiren* s, int32_t velo);
void csr_siren_set_note_from_ext(CsrSiren* s, int32_t note);
void csr_siren_setnote(CsrSiren* s);
float csr_siren_calculwave(CsrSiren* s);
float csr_siren_current_pitch(const CsrSiren* s);
/* The last error on this thread, NUL-terminated into buf; returns its full length. */
size_t sdsp_last_error(uint8_t* buf, size_t len);

#ifdef __cplusplus
}
#endif
