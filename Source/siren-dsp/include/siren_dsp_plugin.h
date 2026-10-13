/* The siren-dsp C ABI the plugin uses with COMPOSESIREN_RUST_DSP (src/plugin.rs):
 * one Rust synthesis per siren voice, driven like the C++ Sirene. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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
/* frames.skip_silent: practically silent frames play the next one (off: the plugin's frames). */
void csr_siren_set_skip_silent(CsrSiren* s, bool on);
/* The state a held note carries from one voice to another (the DSP swap mid-note): the
   rotor's slide, the flap's level, each partial's smoothed amplitude and phase, the frames
   the two notes are on. Same layout as the C++ Sirene::Handoff. Real-time, both ways. */
typedef struct CsrHandoff {
    int32_t note_voulue_avant_slide;
    float note_encour;
    int32_t inter_depart;
    float ampvoulu, ampvouluz;
    int32_t count_k_inf, count_k_sup, count_p_inf, count_p_sup;
    float ampz[200];
    float amp[200];
    double phase_inf[200];
    double phase_sup[200];
    float ancienne_wave_inf;
    double trio_phi;  /* the trio's harmonic phase, 0 elsewhere */
    int32_t trio;     /* read only: 1 when the voice is a trio */
} CsrHandoff;
void csr_siren_get_handoff(const CsrSiren* s, CsrHandoff* out);
void csr_siren_set_handoff(CsrSiren* s, const CsrHandoff* h);
/* The last error on this thread, NUL-terminated into buf; returns its full length. */
size_t sdsp_last_error(uint8_t* buf, size_t len);

#ifdef __cplusplus
}
#endif
