extern "C" {
#include "m_pd.h"
}

#include "CSirenEngine.h"

#include <cstring>
#include <string>

static t_class* c_siren_tilde_class = nullptr;

struct t_c_siren_tilde {
  t_object x_obj;
  t_float f_dummy;
  t_outlet* signal_out;
  CSirenEngine* engine;
  int sampleCounter512;
  int noteTickCounter;
  int samplesPerNoteTick;
};

static std::string default_resource_path() {
#if defined(_WIN32)
  return "C:\\Program Files\\Common Files\\Mecanique Vivante\\ComposeSiren_Orchestra\\Resources\\";
#elif defined(__APPLE__)
  return "/Library/Audio/Plug-Ins/Mecanique Vivante/ComposeSiren_Orchestra/Resources/";
#else
  return "/usr/share/ComposeSiren/Resources/";
#endif
}

static t_int* c_siren_tilde_perform(t_int* w) {
  auto* x = reinterpret_cast<t_c_siren_tilde*>(w[1]);
  auto* out = reinterpret_cast<t_sample*>(w[2]);
  int n = static_cast<int>(w[3]);

  for (int i = 0; i < n; ++i) {
    if (x->sampleCounter512 % 512 == 0) {
      x->engine->tickControlFrame();
    }
    x->sampleCounter512++;

    if (x->noteTickCounter >= x->samplesPerNoteTick) {
      x->engine->tickNoteSlide();
      x->noteTickCounter = 0;
    }
    x->noteTickCounter++;

    out[i] = x->engine->processSample();
  }

  return (w + 4);
}

static void c_siren_tilde_dsp(t_c_siren_tilde* x, t_signal** sp) {
  double sr = sp[0]->s_sr;
  if (sr <= 1000.0) sr = 44100.0;
  x->engine->setSampleRate(sr);
  x->samplesPerNoteTick = static_cast<int>(sr / 1000.0);
  if (x->samplesPerNoteTick < 1) x->samplesPerNoteTick = 1;
  dsp_add(c_siren_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void c_siren_tilde_note(t_c_siren_tilde* x, t_floatarg f_note, t_floatarg f_vel) {
  int note = static_cast<int>(f_note);
  int vel = static_cast<int>(f_vel);
  if (vel > 0) x->engine->noteOn(note, vel);
  else x->engine->noteOff(note);
}

// ctl <valeur> <numCTL> — valeur puis numéro de contrôleur (ComposeSiren)
static void c_siren_tilde_ctl(t_c_siren_tilde* x, t_floatarg f_val, t_floatarg f_cc) {
  x->engine->controlChange(static_cast<int>(f_cc), static_cast<int>(f_val));
}

static void c_siren_tilde_bend(t_c_siren_tilde* x, t_symbol*, int argc, t_atom* argv) {
  if (argc == 1 && argv[0].a_type == A_FLOAT) {
    x->engine->pitchBend14(static_cast<int>(atom_getfloat(argv)));
    return;
  }
  if (argc >= 2 && argv[0].a_type == A_FLOAT && argv[1].a_type == A_FLOAT) {
    int lsb = static_cast<int>(atom_getfloat(argv));
    int msb = static_cast<int>(atom_getfloat(argv + 1));
    int value14 = ((msb & 0x7F) << 7) | (lsb & 0x7F);
    x->engine->pitchBend14(value14);
  }
}

static void c_siren_tilde_reset(t_c_siren_tilde* x) {
  x->engine->reset();
}

static void c_siren_tilde_resources(t_c_siren_tilde* x, t_symbol* s) {
  if (s == nullptr || s == &s_) return;
  std::string error;
  if (!x->engine->setResourcesPath(s->s_name, error)) {
    pd_error(x, "c-siren~: unable to load resources from '%s': %s", s->s_name, error.c_str());
  }
}

static void c_siren_tilde_model(t_c_siren_tilde* x, t_symbol* s) {
  if (s == nullptr || s == &s_) return;
  std::string error;
  if (!x->engine->setModel(s->s_name, error)) {
    pd_error(x, "c-siren~: unable to switch model '%s': %s", s->s_name, error.c_str());
  }
}

static void c_siren_tilde_free(t_c_siren_tilde* x) {
  delete x->engine;
}

static void* c_siren_tilde_new(t_symbol* s, int argc, t_atom* argv) {
  auto* x = reinterpret_cast<t_c_siren_tilde*>(pd_new(c_siren_tilde_class));
  x->f_dummy = 0.0f;
  x->sampleCounter512 = 0;
  x->noteTickCounter = 0;
  x->samplesPerNoteTick = 44;
  x->engine = new CSirenEngine();
  x->signal_out = outlet_new(&x->x_obj, &s_signal);

  std::string model = "alto";
  std::string resources = default_resource_path();

  if (argc >= 1 && argv[0].a_type == A_SYMBOL) {
    model = atom_getsymbol(argv)->s_name;
  }
  if (argc >= 2 && argv[1].a_type == A_SYMBOL) {
    resources = atom_getsymbol(argv + 1)->s_name;
  }

  std::string error;
  if (!x->engine->init(model, resources, 44100.0, error)) {
    pd_error(x, "c-siren~: init failed for model '%s' using '%s': %s", model.c_str(), resources.c_str(), error.c_str());
  }

  post("c-siren~: model=%s resources=%s", x->engine->getModel().c_str(), x->engine->getResourcesPath().c_str());
  return x;
}

extern "C" void c_siren_tilde_setup(void) {
  c_siren_tilde_class = class_new(
      gensym("c-siren~"),
      reinterpret_cast<t_newmethod>(c_siren_tilde_new),
      reinterpret_cast<t_method>(c_siren_tilde_free),
      sizeof(t_c_siren_tilde),
      CLASS_DEFAULT,
      A_GIMME,
      A_NULL);

  CLASS_MAINSIGNALIN(c_siren_tilde_class, t_c_siren_tilde, f_dummy);

  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_dsp), gensym("dsp"), A_CANT, A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_note), gensym("note"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_ctl), gensym("ctl"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_bend), gensym("bend"), A_GIMME, A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_reset), gensym("reset"), A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_resources), gensym("resources"), A_SYMBOL, A_NULL);
  class_addmethod(c_siren_tilde_class, reinterpret_cast<t_method>(c_siren_tilde_model), gensym("model"), A_SYMBOL, A_NULL);
}

// Pd can request setup symbol names derived from the filename.
// Keep explicit aliases to support hyphen/tilde mangling variants.
extern "C" void setup_c_siren_tilde(void) { c_siren_tilde_setup(); }
extern "C" void setup_c0x2dsiren_tilde(void) { c_siren_tilde_setup(); }
