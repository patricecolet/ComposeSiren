#pragma once

#include "Sirene.h"

#include <memory>
#include <string>

class CSirenEngine {
public:
  CSirenEngine();
  ~CSirenEngine();

  bool init(const std::string& modelName, const std::string& resourcesPath, double sampleRate, std::string& error);
  bool setResourcesPath(const std::string& resourcesPath, std::string& error);
  bool setModel(const std::string& modelName, std::string& error);

  void setSampleRate(double sampleRate);
  void noteOn(int note, int velocity);
  void noteOff(int note);
  void controlChange(int ccNumber, int ccValue);
  void pitchBend14(int value14);
  void reset();

  void tickControlFrame();
  void tickNoteSlide();
  float processSample();

  const std::string& getModel() const { return modelName; }
  const std::string& getResourcesPath() const { return resourcesPath; }
  int getMappedMidiChannel() const { return midiChannel; }

private:
  std::string normalizePath(const std::string& path) const;
  std::string normalizeModel(const std::string& raw) const;
  std::string modelToSireneId(const std::string& model) const;
  int modelToMidiChannel(const std::string& model) const;
  float modelMultiplier(const std::string& model) const;

  float tabledeCorrespondanceMidiNote(float note) const;
  void setVitesse(float vitesse);
  void sendVaria();
  void sendVol(int message);
  void createRelease();
  void createRampe();
  void incrementVibrato();

  std::unique_ptr<Sirene> sirene;

  std::string modelName;
  std::string resourcesPath;
  int midiChannel = 1;
  float engineMultiplier = 5.0f;
  double sampleRate = 44100.0;
  float incrementationVibrato = 0.0f;

  float controls[128] = {0.0f};
  float control1Final = 0.0f;
  float noteOnValue = 0.0f;
  float velocity = 0.0f;
  float pitchbend = 0.0f;
  float noteOnFinal = 0.0f;
  float volumeFinal = 0.0f;
  float tourmoteur = 0.0f;
  float varvfo = 0.0f;
  float vartremolo = 0.0f;
  float vitesse = 0.0f;
  float tremolo = 0.0f;
  float vitesseClapet = 0.0f;

  int veloFinal = 0;
  int oldMessage = -1;
  bool isAttackVibrato = false;
  bool isRampe = false;
  bool isRelease = false;
  int countCreateRelease = 0;
  int countCreateAttack = 0;
  int timerDiv9 = 0;

  static constexpr float kEscursionPercent = 10.0f;
};
