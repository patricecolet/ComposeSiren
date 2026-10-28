//
// Created by joseph larralde on 08/03/2026.
//

#include <algorithm>
#include "MidiIn.h"

MidiIn::MidiIn(sirenId sid,
               const std::function<void(float)>& volumeChanged,
               const std::function<void(float)>& noteChanged) :
    id(sid),
    onVolumeChanged(volumeChanged),
    onNoteChanged(noteChanged)
{
    data = sirenPropertiesById.at(id);
    // volet ouvert (CC 7 a 127), comme reset_sirene() du firmware : une voix neuve sonne
    // sans attendre que l'hote envoie le volume
    control[7] = 127;
}

MidiIn::~MidiIn() = default;

void MidiIn::setSirenId(sirenId newSirenId) {
    if (newSirenId == id) return;

    id = newSirenId;
    data = sirenPropertiesById.at(id);
}

//==============================================================================
// From CS_MidiIn (added pitch bend range management (cc16)):

void MidiIn::setSampleRate(double newSampleRate) {
    sampleRate = newSampleRate;
    incrementationVibrato = (512.0 / sampleRate) / 0.025;
}

void MidiIn::timerAudio() {
    // if (isWithSynth) {
    // for (int Ch = 1; Ch < 9; Ch++) {
    sendVaria();

    if (isRampe) {
        createRampe();
    }

    if (isRelease) {
        createRelease();
    }

    if (isAttackVibrato && countTimerAudio == 0) {
        incrementeVibrato();
    }
    // }

    // this business logic might be moved in an owner class instance owning
    // several sirens, but whatever ... no big difference for now
    countTimerAudio++;

    if (countTimerAudio >= 9) {
        countTimerAudio = 0;
    }
    // }
}

float MidiIn::tourMoteurFromNote(float note) const {
    float midiNote = std::max<float>(0, note);
    float res = 0.;

    float freq = 440.0f * powf(2., (midiNote - 81.0f) / 12.0f);
    if (freq > 8.) {
        res = freq * data->synthConstants.engineSpeedFrequencyRatio;
    }
    return res;
}

float MidiIn::noteFromVitesse(float speed) const {
    int midiCent = 0;
    float ratio = data->synthConstants.engineSpeedFrequencyRatio;
    midiCent = (int) roundf(
        (69.f + 12.f * log2f((speed / ratio) / 440.0f)) * 100.0f
    );
    return std::max(0, midiCent);
}

void MidiIn::realTimeStartNote(int note, int velo) {
    // if (Ch == 1) countVibra = 0; (???)
    countVibra = 0;
    if ((control[1] != 0 && control[9] != 0 && control[11] != 0 && velo > 0 && velocite == 0)
        ||(control[1] != 0 && control[9] != 0 && control[11] != 0 && velo > 0 && note != noteOn)) {
        controlFinal = 0;
        isAttackVibrato = true;
    }
    noteOn = note;
    velocite = velo;
    noteOnFinal = noteOn + pitchBend;
    volumeFinal = velocite * (control[7] / 127.0) * (500. / 127.);
    volumeFinal = std::clamp<float>(volumeFinal, 0., 500.);
    tourMoteur = tourMoteurFromNote(noteOnFinal);
    sendVaria();
    sendVol((int) (volumeFinal * changeVolumeGeneral));
}

void MidiIn::realTimeStopNote(int note) {
    if (note == noteOn) {
        sendVaria();
        sendVol(0);
        velocite = 0.0;
        volumeFinal = 0.0;
    }
}

void MidiIn::handleControlChange(int cc, int value) {
    // parameters not taken into account :
    // - sourdine (cc 12)
    // - timbre (cc 13)
    // - pitch bend range (cc 16)
    // - effect order switch (cc 42) but this one is not implemented in firmware
    switch (cc) {
#if COMPOSESIREN_RESETALLCONTROLLERS
        case 120: // All Sound Off (standard MIDI)
            if (acceptsChannelMode(cc, value)) {
                onMidiAllSoundsOff();
            }
            break;
#endif
        case 121: // Reset All Controllers
#if COMPOSESIREN_RESETALLCONTROLLERS
            if (!acceptsChannelMode(cc, value)) break;
#endif
            resetSirene();
            break;
#if COMPOSESIREN_RESETALLCONTROLLERS
        case 123: // All Notes Off (standard MIDI)
            if (acceptsChannelMode(cc, value)) {
                onMidiAllNotesOff();
            }
            break;
#endif
        case 1 : { // vibrato depth
            control[1] = value ;
            if (control[11] == 0) {
                controlFinal = control[1];
            }
            if (control[1] == 0 && isAttackVibrato) {
                isAttackVibrato = false;
            }
            break;
        }
        case 5 : // portamento
            control[5] = value;
            break;
        case 6 : // ??????????
            control[6] = value;
            break;
        case 7 : { // volume
            control[7] = value;
            volumeFinal = velocite * (control[7] / 127.0) * (500. / 127.);
            volumeFinal = std::clamp<float>(volumeFinal, 0.0, 500.0);
            sendVol((int) (volumeFinal * changeVolumeGeneral));
            break;
        }
        case 9 : { // vibrato vitesse
            control[9] = value ;
            if (control[9] < 0.0)   control[9] = 0.0;
            if (control[9] > 127.0) control[9] = 127.0;
            if (control[9] == 0 && isAttackVibrato) {
                isAttackVibrato = false;
            }
            break;
        }
        case 11 : { // vibrato acceleration
            control[11] = value ;
            if (control[11] == 0 && isAttackVibrato) {
                isAttackVibrato = false;
            }
            break;
        }
        case 15 : { // tremolo vitesse
            control[15] = value ;
            if (value == 0) varTremolo = 0;
            break;
        }
        case 16: // pitch bend range
            control[16] = value;
            break;
        case 72 : // release
            control[72] = value;
            break;
        case 73 : // attack
            control[73] = value;
            break;
        case 92 : // tremolo profondeur
            control[92] = value;
            break;
        default:
            break;
    }
}

void MidiIn::handlePitchWheel(int lsb, int msb) {
    pitchBend = (msb << 7) | lsb;
    pitchBend = (pitchBend - 8192) / 8192.;
    int pitchBendRange = control[16];
    noteOnFinal = noteOn + pitchBend * pitchBendRange;
    tourMoteur = tourMoteurFromNote(noteOnFinal);
}

void MidiIn::sendVaria() {
    float vibrato = 0.;

    // <= valeur 2 * PI (pour le calcul du sinus)
    // et frequence de modulation different de 0
    if ((varVfo <= 628) && (control[9] != 0) && (control[1] != 0)) {
        // valeur frequence = f*10  12,7Hz = 127 période = 1/freq
        varVfo = varVfo + incrementationVibrato * control[9];
        vibrato = ((tourMoteur * constescursion * controlFinal) / 12700.) * std::sin(varVfo / 100.);
    } else {
        varVfo = 0;
        vibrato = 0.;
    }

    // <= valeur 2 * PI (pour le calcul du sinus
    //  et frequence de tremolo different de 0
    if ((varTremolo <= 628) && (control[15]!=0) && (control[92] != 0)) {
        // valeur frequence = f*10  12,7Hz = 127 periode = 1/freq
        varTremolo = varTremolo + incrementationVibrato * control[15];
    } else {
        varTremolo = 0;
    }

    if (control[15] != 0 && control[92] != 0 && !isRelease && !isRampe && !isEnVeille) {
        int volume = (int)(volumeFinal * changeVolumeGeneral);
        tremolo = volume - (
            ((volume * std::sin(varTremolo / 100.)) / (256. / control[92])) +
            (volume / (256. / control[92]))
        );
        sendVol(volume);
    }

    //***************************** Portamento *******************************//
    if (control[5] == 0.0) {
        vitesse = tourMoteur;
    } else {
        float nbr = ((control[5] / 127.) / 5.) + 0.80;
        vitesse = (1.-nbr) * tourMoteur + nbr * vitesse;
        //************************ end Portamento ****************************//
    }

    float note = noteFromVitesse(vitesse + vibrato);

    //********************** EXECUTE CALLBACK :
    // onEngineSpeedChanged(Ch, vitesse + vibrato);
    onNoteChanged(note);
}

void MidiIn::sendVol(int message) {
    if (control[73] > 0.0 && message >= 2 && ancienVelo <= 1) { // Attack
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
    }
    else if(control[72] > 0.0 && message <= 1 && ancienVelo >= 2) { // Release
        if (isRelease) {
            isRelease = false;
            countCreateRelease--;
        }
        if (isRampe) {
            isRampe = false;
            countCreateAttack--;
        }
        isRelease = true;
    }
    else {
        if (isRampe && message <= 1 ) {
            isRampe = false;
            countCreateAttack--;
        }
        if (isRelease && message > 1) {
            isRelease = false;
            countCreateRelease--;
        }
        if(!isRampe && !isRelease) {
            if (control[15] > 0. && control[92] > 0.) {
                vitesseClapet = veloFinal = (int) tremolo;
            } else {
                vitesseClapet = veloFinal = message;
            }

            // if (isWithSynth && Ch !=8) {
                //********************** EXECUTE CALLBACK :
                // onVelocityChanged(Ch, veloFinal[Ch]);
            // }
            onVolumeChanged(veloFinal);
        }
    }
    ancienVelo = message;
}

void MidiIn::createRampe() {
    int vitesseVoulue = volumeFinal * changeVolumeGeneral;
    float nbr = (128 - control[73]) / 7.62;
    vitesseClapet = vitesseClapet + nbr;
    int around = (int) vitesseClapet;
    if (around >= vitesseVoulue) {
        if (isRampe) {
            isRampe = false;
            countCreateAttack--;
        }
        veloFinal = vitesseClapet = around = vitesseVoulue;
    }
    if (control[15] !=0. && control[92] !=0.) {
        tremolo = around - (((around * std::sin(varTremolo / 100.)) / (256. / control[92])) + (around / (256. / control[92])));
        veloFinal = (int) tremolo;
    } else {
        veloFinal = around;
    }

    // if(isWithSynth && Ch !=8) {
    //   ********************** EXECUTE CALLBACK :
    //   onVelocityChanged(Ch, veloFinal[Ch]);
    // }

    onVolumeChanged(veloFinal);
}

void MidiIn::createRelease(){
    float nbr = 128 - control[72];
    if (vitesseClapet >= 250) nbr /= 7.62;
    else if (vitesseClapet < 250 && vitesseClapet >= 200) nbr /= 10.;
    else if (vitesseClapet < 200 && vitesseClapet >= 150) nbr /= 15.;
    else if (vitesseClapet < 150 && vitesseClapet >= 100) nbr /= 20;
    else if (vitesseClapet < 100 && vitesseClapet >= 50) nbr /= 25.;
    else if (vitesseClapet < 50) nbr /= 30.;
    vitesseClapet -= nbr;
    int around = (int) vitesseClapet;
    if (around <= 1) {
        if (isRelease) {
            isRelease = false;
        }
        countCreateRelease--;
        tremolo = veloFinal = around = 0;
    } else if (control[15] != 0. && control[92] != 0.) {
        tremolo = around - (((around * std::sin(varTremolo / 100.)) / (256. / control[92])) + (around / (256. / control[92])));
        veloFinal = (int) tremolo;
    } else veloFinal = around;

    // if (isWithSynth && Ch !=8) {
    //   ********************** EXECUTE CALLBACK :
    //   onVelocityChanged(Ch, veloFinal[Ch]);
    // }

    onVolumeChanged(veloFinal);
}

void MidiIn::incrementeVibrato() {
    if (controlFinal < control[1]) {
        controlFinal += (control[11] / 12.7);
    } else {
        controlFinal = control[1];
        isAttackVibrato = false;
    }
}

void MidiIn::stOffVariateur() {
    isEnVeille = true;
}

void MidiIn::stOnVariateur() {
    isEnVeille = false;
}

void MidiIn::stopSirene() {
    // like resetSirene but keeping cc values
    noteOnFinal = 0.0;
    ancienVolFinal = -1;

    noteOn = 0.;
    velocite = 0;
    tourMoteur = 0.0;
    noteOnFinal = 0.0;
    volumeFinal = 0.0;

    varVfo = 0;
    varTremolo = 0;
    vitesse = 0;
    tremolo = 0;

    isAttackVibrato = false;

    if (isRampe) {
        isRampe = false;
        countCreateAttack--;
    }

    if (isRelease) {
        isRelease = false;
        countCreateRelease--;
        // isRelease = false; // twice ?
    }

    veloFinal = 500;
    volumeFinal = 500.;
    vitesseClapet = 0;
    ancienVelo = -1;
    ancienVolFinal = -1;
}

#if COMPOSESIREN_RESETALLCONTROLLERS
void MidiIn::onMidiAllNotesOff() {
    // MIDI 1.0 Detailed Specification 4.2.1, appendix A-5: voices should
    // "go to the release stage of the envelope"; hold/sustain takes priority.
    // https://midi.org/midi-1-0-detailed-specification
    // The official controller table specifies CC 123 value 0:
    // https://midi.org/midi-1-0-control-change-messages
    // Here only the current monophonic note is released. Sustain and full
    // Omni/Basic Channel mode handling are not implemented; the configurable
    // value rule retains positive-value button compatibility by default.
    if (velocite > 0.0) {
        realTimeStopNote(static_cast<int>(noteOn));
    }
}

void MidiIn::onMidiAllSoundsOff() {
    // MIDI 1.0 Detailed Specification 4.2.1, printed p. 25: sounding notes'
    // "volume envelopes are set to zero as soon as possible".
    // https://midi.org/midi-1-0-detailed-specification
    // The official controller table specifies CC 120 value 0:
    // https://midi.org/midi-1-0-control-change-messages
    // Cancel attack/release and request zero MIDI volume, keeping controllers.
    // Sirene's flap interpolation/leakage and downstream reverb remain; this
    // callback alone does not guarantee immediate silence at the audio output.
    if (isRampe) {
        isRampe = false;
        countCreateAttack--;
    }
    if (isRelease) {
        isRelease = false;
        countCreateRelease--;
    }
    velocite = 0.0;
    volumeFinal = 0.0;
    tremolo = 0.0;
    veloFinal = 0;
    vitesseClapet = 0;
    ancienVelo = 0;
    onVolumeChanged(0);
}

#endif
void MidiIn::resetSirene() {
#if COMPOSESIREN_RESETALLCONTROLLERS
    // Request zero MIDI volume before restoring the controller map.
    if (isControllerResetEnabled())
        onMidiAllSoundsOff();

#endif
    noteOnFinal = 0.0;
    ///////////////////////////////////////////////////****** Ferme les volets
    ancienVolFinal = -1;
    control[1] = 0;
    control[5] = 0;
    control[9] = 0;
    control[11] = 0;
    control[15] = 0;
    control[17] = 100; // as reset_sirene() (m_seq/s_un_in.c)
    control[18] = 0;
    control[20] = 0;
    control[21] = 0;
    control[22] = 0;
    control[23] = 0;
    control[92] = 0;
    control[72] = 0;
    control[73] = 0;
    LSB = 0;
    MSB = 64;
    pitchBend = 0.;
    noteOn = 0.;
    velocite = 0;
    tourMoteur = 0.0;
    noteOnFinal = 0.0;
    volumeFinal = 0.0;
#if COMPOSESIREN_RESETALLCONTROLLERS
    if (isControllerResetEnabled())
        controlFinal = 0;
#endif
    control[6] = 64;
    control[12] = 127.;
    control[13] = 127.;
    // why this ?
    // control1Final[16] = 0.; // (???) are there some kind of proxy channels ?
    varVfo = 0;
    varTremolo = 0;
    vitesse = 0;
    tremolo = 0;

    isAttackVibrato = false;

    if (isRampe) {
        isRampe = false;
        countCreateAttack--;
    }

    if (isRelease) {
        isRelease = false;
        countCreateRelease--;
        // isRelease = false; // twice ?
    }

    control[7] = 127; // ouverture du volet max (90°)
    veloFinal = 500;
    volumeFinal = 500.;
    vitesseClapet = 0;
    ancienVelo = -1;
    ancienVolFinal = -1;
}

void MidiIn::copyStateFrom(const MidiIn& o) {
    changeVolumeGeneral = o.changeVolumeGeneral;
    noteOn = o.noteOn;
    velocite = o.velocite;
    pitchBend = o.pitchBend;
    std::copy(std::begin(o.control), std::end(o.control), std::begin(control));
    controlFinal = o.controlFinal;
    noteOnFinal = o.noteOnFinal;
    volumeFinal = o.volumeFinal;
    tourMoteur = o.tourMoteur;
    LSB = o.LSB;
    MSB = o.MSB;
    varVfo = o.varVfo;
    varTremolo = o.varTremolo;
    isEnVeille = o.isEnVeille;
    vitesse = o.vitesse;
    tremolo = o.tremolo;
    veloFinal = o.veloFinal;
    vitesseClapet = o.vitesseClapet;
    ancienVelo = o.ancienVelo;
    ancienVolFinal = o.ancienVolFinal;
    countCreateRelease = o.countCreateRelease;
    countCreateAttack = o.countCreateAttack;
    constescursion = o.constescursion;
    sampleRate = o.sampleRate;
    incrementationVibrato = o.incrementationVibrato;
    countTimerAudio = o.countTimerAudio;
    countVibra = o.countVibra;
    isAttackVibrato = o.isAttackVibrato;
    isRampe = o.isRampe;
    isRelease = o.isRelease;
}
