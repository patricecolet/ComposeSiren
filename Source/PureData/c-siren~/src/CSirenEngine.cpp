#include "CSirenEngine.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>

template <typename T>
static T clampValue(T value, T lo, T hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

CSirenEngine::CSirenEngine() {
  controls[7] = 127.0f;
  controls[12] = 127.0f;
  controls[13] = 127.0f;
  controls[6] = 60.0f;
  setSampleRate(44100.0);
}

CSirenEngine::~CSirenEngine() {}

bool CSirenEngine::init(
    const std::string& requestedModel,
    const std::string& requestedResourcesPath,
    double requestedSampleRate,
    std::string& error) {
  modelName = normalizeModel(requestedModel);
  midiChannel = modelToMidiChannel(modelName);
  engineMultiplier = modelMultiplier(modelName);
  resourcesPath = normalizePath(requestedResourcesPath);
  setSampleRate(requestedSampleRate);
  reset();

  try {
    sirene = std::make_unique<Sirene>(modelToSireneId(modelName), resourcesPath);
    sirene->setSampleRate(sampleRate);
  } catch (const std::exception& ex) {
    error = ex.what();
    sirene.reset();
    return false;
  }
  return true;
}

bool CSirenEngine::setResourcesPath(const std::string& requestedResourcesPath, std::string& error) {
  return init(modelName, requestedResourcesPath, sampleRate, error);
}

bool CSirenEngine::setModel(const std::string& requestedModel, std::string& error) {
  return init(requestedModel, resourcesPath, sampleRate, error);
}

void CSirenEngine::setSampleRate(double newSampleRate) {
  if (newSampleRate <= 1000.0) {
    newSampleRate = 44100.0;
  }
  sampleRate = newSampleRate;
  incrementationVibrato = static_cast<float>((512.0 / sampleRate) / 0.025);
  if (sirene) {
    sirene->setSampleRate(sampleRate);
  }
}

void CSirenEngine::noteOn(int note, int vel) {
  note = clampValue(note, 0, 127);
  vel = clampValue(vel, 0, 127);

  if (vel == 0) {
    noteOff(note);
    return;
  }

  if ((controls[1] != 0 && controls[9] != 0 && controls[11] != 0 && velocity == 0) ||
      (controls[1] != 0 && controls[9] != 0 && controls[11] != 0 && note != noteOnValue)) {
    control1Final = 0;
    isAttackVibrato = true;
  }

  noteOnValue = static_cast<float>(note);
  velocity = static_cast<float>(vel);
  noteOnFinal = noteOnValue + pitchbend;
  volumeFinal = (velocity * (controls[7] / 127.0f)) * (500.0f / 127.0f);
  volumeFinal = clampValue(volumeFinal, 0.0f, 500.0f);
  tourmoteur = tabledeCorrespondanceMidiNote(noteOnFinal);
  sendVaria();
  sendVol(static_cast<int>(volumeFinal));
}

void CSirenEngine::noteOff(int note) {
  if (note == static_cast<int>(noteOnValue)) {
    sendVaria();
    sendVol(0);
    velocity = 0.0f;
    volumeFinal = 0.0f;
  }
}

void CSirenEngine::controlChange(int ccNumber, int ccValue) {
  ccNumber = clampValue(ccNumber, 0, 127);
  ccValue = clampValue(ccValue, 0, 127);

  switch (ccNumber) {
    case 121:
      reset();
      break;
    case 1:
      controls[1] = static_cast<float>(ccValue);
      if (controls[11] == 0) control1Final = controls[1];
      if (controls[1] == 0 && isAttackVibrato) isAttackVibrato = false;
      break;
    case 5:
      controls[5] = static_cast<float>(ccValue);
      break;
    case 6:
      controls[6] = static_cast<float>(ccValue);
      break;
    case 7:
      controls[7] = static_cast<float>(ccValue);
      volumeFinal = (velocity * (controls[7] / 127.0f)) * (500.0f / 127.0f);
      volumeFinal = clampValue(volumeFinal, 0.0f, 500.0f);
      sendVol(static_cast<int>(volumeFinal));
      break;
    case 9:
      controls[9] = static_cast<float>(ccValue);
      if (controls[9] == 0 && isAttackVibrato) isAttackVibrato = false;
      break;
    case 11:
      controls[11] = static_cast<float>(ccValue);
      if (controls[11] == 0 && isAttackVibrato) isAttackVibrato = false;
      break;
    case 15:
      controls[15] = static_cast<float>(ccValue);
      if (ccValue == 0) vartremolo = 0;
      break;
    case 72:
      controls[72] = static_cast<float>(ccValue);
      break;
    case 73:
      controls[73] = static_cast<float>(ccValue);
      break;
    case 92:
      controls[92] = static_cast<float>(ccValue);
      break;
    default:
      controls[ccNumber] = static_cast<float>(ccValue);
      break;
  }
}

void CSirenEngine::pitchBend14(int value14) {
  value14 = clampValue(value14, 0, 16383);
  pitchbend = static_cast<float>(value14 - 8192) / 8192.0f;
  noteOnFinal = noteOnValue + pitchbend;
  tourmoteur = tabledeCorrespondanceMidiNote(noteOnFinal);
}

void CSirenEngine::reset() {
  controls[1] = 0;
  controls[5] = 0;
  controls[9] = 0;
  controls[11] = 0;
  controls[15] = 0;
  controls[17] = 0;
  controls[18] = 0;
  controls[72] = 0;
  controls[73] = 0;
  controls[92] = 0;
  controls[7] = 127;
  controls[12] = 127;
  controls[13] = 127;
  controls[6] = 60;

  control1Final = 0.0f;
  noteOnValue = 0.0f;
  velocity = 0.0f;
  pitchbend = 0.0f;
  noteOnFinal = 0.0f;
  volumeFinal = 500.0f;
  tourmoteur = 0.0f;
  varvfo = 0.0f;
  vartremolo = 0.0f;
  vitesse = 0.0f;
  tremolo = 0.0f;
  vitesseClapet = 0.0f;

  veloFinal = 500;
  oldMessage = -1;
  isAttackVibrato = false;
  isRampe = false;
  isRelease = false;
  countCreateRelease = 0;
  countCreateAttack = 0;
  timerDiv9 = 0;

  if (sirene) {
    sirene->setVelocite(0);
    sirene->setnoteFromExt(0);
  }
}

void CSirenEngine::tickControlFrame() {
  sendVaria();
  if (isRampe) {
    createRampe();
  }
  if (isRelease) {
    createRelease();
  }
  if (isAttackVibrato && timerDiv9 == 0) {
    incrementVibrato();
  }
  timerDiv9++;
  if (timerDiv9 >= 9) timerDiv9 = 0;
}

void CSirenEngine::tickNoteSlide() {
  if (sirene) {
    sirene->setnote();
  }
}

float CSirenEngine::processSample() {
  if (!sirene) {
    return 0.0f;
  }
  return sirene->calculwave();
}

std::string CSirenEngine::normalizePath(const std::string& path) const {
  if (path.empty()) return path;
  std::string out = path;
  char last = out.back();
  if (last != '/' && last != '\\') {
    out.push_back('/');
  }
  return out;
}

std::string CSirenEngine::normalizeModel(const std::string& raw) const {
  std::string model = raw;
  std::transform(model.begin(), model.end(), model.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  if (model == "s1" || model == "alto") return "alto";
  if (model == "s3" || model == "bass") return "bass";
  if (model == "s4" || model == "tenor") return "tenor";
  if (model == "s5" || model == "s6" || model == "soprano") return "soprano";
  if (model == "s7" || model == "piccolo") return "piccolo";
  return "alto";
}

std::string CSirenEngine::modelToSireneId(const std::string& model) const {
  if (model == "bass") return "S3";
  if (model == "tenor") return "S4";
  if (model == "soprano") return "S5";
  if (model == "piccolo") return "S7";
  return "S1";
}

int CSirenEngine::modelToMidiChannel(const std::string& model) const {
  if (model == "bass") return 3;
  if (model == "tenor") return 4;
  if (model == "soprano") return 5;
  if (model == "piccolo") return 7;
  return 1;
}

float CSirenEngine::modelMultiplier(const std::string& model) const {
  if (model == "alto") return 5.0f;
  if (model == "tenor") return 20.0f / 3.0f;
  return 7.5f;
}

float CSirenEngine::tabledeCorrespondanceMidiNote(float note) const {
  float midiNote = note;
  if (midiNote < 0.0f) midiNote = 0.0f;
  float freq2 = 440.0f * std::pow(2.0f, (midiNote - 81.0f) / 12.0f);
  if (freq2 > 8.0f) {
    return freq2 * engineMultiplier;
  }
  return 0.0f;
}

void CSirenEngine::setVitesse(float rawVitesse) {
  if (!sirene) return;
  if (rawVitesse <= 0.0f) {
    sirene->setnoteFromExt(0);
    return;
  }

  float engineFreq = rawVitesse / engineMultiplier;
  if (engineFreq <= 0.0f) {
    sirene->setnoteFromExt(0);
    return;
  }

  int midicent = static_cast<int>(std::round((69.0 + 12.0 * std::log2(engineFreq / 440.0)) * 100.0));
  if (midicent < 0) midicent = 0;
  sirene->setnoteFromExt(midicent);
}

void CSirenEngine::sendVaria() {
  float vibrato = 0.0f;
  if ((varvfo <= 628.0f) && (controls[9] != 0) && (controls[1] != 0)) {
    varvfo = varvfo + incrementationVibrato * controls[9];
    vibrato = (((tourmoteur * kEscursionPercent * control1Final) / 12700.0f) * std::sin(varvfo / 100.0f));
  } else {
    varvfo = 0;
    vibrato = 0;
  }

  if ((vartremolo <= 628.0f) && (controls[15] != 0) && (controls[92] != 0)) {
    vartremolo = vartremolo + incrementationVibrato * controls[15];
  } else {
    vartremolo = 0;
  }

  if (controls[15] != 0 && controls[92] != 0 && !isRelease && !isRampe) {
    int volume = static_cast<int>(volumeFinal);
    tremolo = volume - (((volume * std::sin(vartremolo / 100.0f)) / (256.0f / controls[92])) + (volume / (256.0f / controls[92])));
    sendVol(volume);
  }

  if (controls[5] == 0.0f) {
    vitesse = tourmoteur;
  } else {
    float nbr = ((controls[5] / 127.0f) / 5.0f) + 0.80f;
    vitesse = ((1.0f - nbr) * tourmoteur) + (nbr * vitesse);
  }
  setVitesse(vitesse + vibrato);
}

void CSirenEngine::sendVol(int message) {
  if ((controls[73] > 0.0f) && (message >= 2) && (oldMessage <= 1)) {
    if (isRampe) {
      isRampe = false;
      countCreateAttack--;
    }
    if (isRelease) {
      isRelease = false;
      countCreateRelease--;
    }
    countCreateAttack++;
    if (countCreateAttack == 1) {
      isRampe = true;
    } else {
      countCreateAttack--;
    }
  } else if ((controls[72] > 0.0f) && (message <= 1) && (oldMessage >= 2)) {
    if (isRelease) {
      isRelease = false;
      countCreateRelease--;
    }
    if (isRampe) {
      isRampe = false;
      countCreateAttack--;
    }
    isRelease = true;
  } else {
    if (isRampe && message <= 1) {
      isRampe = false;
      countCreateAttack--;
    }
    if (isRelease && message > 1) {
      isRelease = false;
      countCreateRelease--;
    }
    if (!isRampe && !isRelease) {
      if (controls[15] > 0.0f && controls[92] > 0.0f) {
        vitesseClapet = static_cast<float>(veloFinal = static_cast<int>(tremolo));
      } else {
        vitesseClapet = static_cast<float>(veloFinal = message);
      }
      if (sirene) {
        sirene->setVelocite(veloFinal);
      }
    }
  }
  oldMessage = message;
}

void CSirenEngine::createRelease() {
  float nbr = 128.0f - controls[72];
  if (vitesseClapet >= 250.0f) nbr = nbr / 7.62f;
  else if (vitesseClapet >= 200.0f) nbr = nbr / 10.0f;
  else if (vitesseClapet >= 150.0f) nbr = nbr / 15.0f;
  else if (vitesseClapet >= 100.0f) nbr = nbr / 20.0f;
  else if (vitesseClapet >= 50.0f) nbr = nbr / 25.0f;
  else nbr = nbr / 30.0f;

  vitesseClapet = vitesseClapet - nbr;
  int around = static_cast<int>(vitesseClapet);
  if (around <= 1) {
    if (isRelease) {
      isRelease = false;
    }
    countCreateRelease--;
    tremolo = 0;
    veloFinal = 0;
    around = 0;
  } else if (controls[15] != 0.0f && controls[92] != 0.0f) {
    tremolo = around - (((around * std::sin(vartremolo / 100.0f)) / (256.0f / controls[92])) + (around / (256.0f / controls[92])));
    veloFinal = static_cast<int>(tremolo);
  } else {
    veloFinal = around;
  }
  if (sirene) {
    sirene->setVelocite(veloFinal);
  }
}

void CSirenEngine::createRampe() {
  int vitesseVoulue = static_cast<int>(volumeFinal);
  float nbr = (128.0f - controls[73]) / 7.62f;
  vitesseClapet = vitesseClapet + nbr;
  int around = static_cast<int>(vitesseClapet);
  if (around >= vitesseVoulue) {
    if (isRampe) {
      isRampe = false;
      countCreateAttack--;
    }
    veloFinal = around = vitesseVoulue;
    vitesseClapet = static_cast<float>(vitesseVoulue);
  }

  if (controls[15] != 0.0f && controls[92] != 0.0f) {
    tremolo = around - (((around * std::sin(vartremolo / 100.0f)) / (256.0f / controls[92])) + (around / (256.0f / controls[92])));
    veloFinal = static_cast<int>(tremolo);
  } else {
    veloFinal = around;
  }

  if (sirene) {
    sirene->setVelocite(veloFinal);
  }
}

void CSirenEngine::incrementVibrato() {
  if (control1Final < controls[1]) {
    control1Final = control1Final + (controls[11] / 12.7f);
  } else {
    control1Final = controls[1];
    isAttackVibrato = false;
  }
}
