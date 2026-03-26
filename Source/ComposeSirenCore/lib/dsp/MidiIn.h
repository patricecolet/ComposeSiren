//
// Created by joseph larralde on 08/03/2026.
//

#ifndef COMPOSESIREN_MIDIIN_H
#define COMPOSESIREN_MIDIIN_H


#include "../definitions/parameterDefinitions.h"
#include "../dsp/Sirene.h"

class MidiIn {
private:
	sirenId id;
	std::shared_ptr<const sirenData> data;

public:
    MidiIn(sirenId id,
           const std::function<void(float)>& volumeChanged,
           const std::function<void(float)>& noteChanged);
    ~MidiIn();

	void setSirenId(sirenId id);

    void realTimeStartNote(int note, int velo);
    void realTimeStopNote(int note);
    void handleControlChange(int cc, int value);
    void handlePitchWheel(int lsb, int msb);

    void timerAudio();
    void setSampleRate(double newSampleRate);
    void stopSirene();
    void resetSirene();

private:
	// per-siren midi message management, refactored from MidiIn class :

	// ---------- will be called from above
	// void handleMIDIMessage2(int Ch, int value1, int value2);

	// ---------- safely removable, don't seem to be used
	// void definiMuteEthernet(bool ismuted, int Ch);
	// void JouerClic(int value);
	// void isWithSound(bool is);
	// void changingvolumeclic(int VolumeClic);
	// void sirenium_in(unsigned char *buf);

	// ancien tabledecorresponcanceMidinote()
	float tourMoteurFromNote(float note) const;
	// moved from Synth::setVitesse(int ch, float vitesse)
	float noteFromVitesse(float speed) const;

	void sendVaria();
	void sendVol(int message);

	void createRampe();
	void createRelease();

	void incrementeVibrato();

	// arret d'alimentation du moteur -> embrayage
	void stOffVariateur();	// -> le moteur est reasservi
	void stOnVariateur();	// -> le moteur est desasservi

	// those vars were arrays of size 17 (all 16 channels + 1 extra, or index 0 )
	float changeVolumeGeneral = 1;
	float noteOn = 0;
	float velocite = 0;
	float pitchBend = 0;
	float control[127] = {0};
	float controlFinal = 0;
	float noteOnFinal = 0;
	float volumeFinal = 0;
	float tourMoteur = 0;
	int LSB = 0;
	int MSB = 0;
	float varVfo = 0;
	float varTremolo = 0;
	bool isEnVeille = false;
	float vitesse = 0;
	float tremolo = 0;
	int veloFinal = 0;
	float vitesseClapet = 0;
	int ancienVelo = 0;
	int ancienVolFinal = -1;
	int countCreateRelease = 0;
	int countCreateAttack = 0;

	// valeur maxi de l'amplitude VFO en % de la vitesse correspondant a une note
	// was #define constescursion 10
	float constescursion = 10;

	// those already were simple numbers
	double sampleRate = 44100;
	float incrementationVibrato = (512.0 / sampleRate) / 0.025;
	int countTimerAudio = 0;
	int countVibra = 0;
	// int pitch_bend; // not used

	// those were arrays of size 9 (7 sirens + channels 8 & 9)
	bool isAttackVibrato = false;
	bool isRampe = false;
	bool isRelease = false;

    const std::function<void(float)> onVolumeChanged;
    const std::function<void(float)> onNoteChanged;
};

#endif //COMPOSESIREN_MIDIIN_H