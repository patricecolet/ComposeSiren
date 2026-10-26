/* The reverb-dsp C ABI (src/ffi.rs): ComposeSiren's Reverb and mareverbe in Rust. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsrReverb CsrReverb;
typedef struct CsrMareverbe CsrMareverbe;

uint32_t csr_reverb_abi_version(void);

CsrReverb* csr_reverb_new(void);
void csr_reverb_free(CsrReverb* r);
/* mode: 0 = Plugin (bit-exact with C++ Reverb), 1 = StereoTime */
void csr_reverb_set_mode(CsrReverb* r, int32_t mode);
void csr_reverb_set_sample_rate(CsrReverb* r, double sample_rate);
void csr_reverb_begin_process_block(CsrReverb* r);
void csr_reverb_enable(CsrReverb* r, bool e);
void csr_reverb_set_dry_wet(CsrReverb* r, float v);
void csr_reverb_set_damping(CsrReverb* r, float v);
void csr_reverb_set_room_size(CsrReverb* r, float v);
void csr_reverb_set_width(CsrReverb* r, float v);
void csr_reverb_set_low_cut(CsrReverb* r, float v);
void csr_reverb_set_high_cut(CsrReverb* r, float v);
void csr_reverb_mute(CsrReverb* r);
void csr_reverb_process(
    CsrReverb* r,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    size_t n);

CsrMareverbe* csr_mareverbe_new(void);
void csr_mareverbe_free(CsrMareverbe* m);
void csr_mareverbe_mute(CsrMareverbe* m);
void csr_mareverbe_set_room_size(CsrMareverbe* m, float value);
float csr_mareverbe_get_room_size(const CsrMareverbe* m);
void csr_mareverbe_set_damp(CsrMareverbe* m, float value);
float csr_mareverbe_get_damp(const CsrMareverbe* m);
void csr_mareverbe_set_wet(CsrMareverbe* m, float value);
float csr_mareverbe_get_wet(const CsrMareverbe* m);
void csr_mareverbe_set_dry(CsrMareverbe* m, float value);
float csr_mareverbe_get_dry(const CsrMareverbe* m);
void csr_mareverbe_set_width(CsrMareverbe* m, float value);
float csr_mareverbe_get_width(const CsrMareverbe* m);
void csr_mareverbe_set_mode(CsrMareverbe* m, float value);
float csr_mareverbe_get_mode(const CsrMareverbe* m);
void csr_mareverbe_process_stereo(
    CsrMareverbe* m,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    size_t n);

#ifdef __cplusplus
}
#endif
