//! A port of the plugin's whole `MidiIn` (Source/ComposeSirenCore/lib/dsp/MidiIn.cpp):
//! note on and off, the pitch wheel, and the CCs it reads: 1 vibrato depth,
//! 5 portamento, 7 volume, 9 vibrato speed, 11 vibrato attack, 15 tremolo
//! speed, 16 pitch-bend range, 72 release, 73 attack, 92 tremolo depth,
//! 121 reset. `timerAudio()`, every 512 samples, moves vibrato, tremolo,
//! portamento and the flap's attack and release ramps.
//!
//! Sample-exact with the C++ (`tests/midi_in.rs`, against `oracle play`): the
//! float/double promotions are written out where the C++ makes them. The
//! C++'s two callbacks (`onNoteChanged`, `onVolumeChanged`) are a [`Sink`].

use crate::siren::SirenDef;

/// Where `MidiIn` sends what it computes: the midicent the rotor should go to
/// (`setnoteFromExt`), and the flap's volume (`setVelocite`), both as floats
/// as the C++ passes them.
pub trait Sink {
    fn note(&mut self, note: f32);
    fn volume(&mut self, volume: f32);
}

pub struct MidiIn {
    ratio: f32,
    change_volume_general: f32,
    note_on: f32,
    velocite: f32,
    pitch_bend: f32,
    control: [f32; 127],
    control_final: f32,
    note_on_final: f32,
    volume_final: f32,
    tour_moteur: f32,
    var_vfo: f32,
    var_tremolo: f32,
    is_en_veille: bool,
    vitesse: f32,
    tremolo: f32,
    velo_final: i32,
    vitesse_clapet: f32,
    ancien_velo: i32,
    count_create_attack: i32,
    constescursion: f32,
    incrementation_vibrato: f32,
    count_timer_audio: i32,
    is_attack_vibrato: bool,
    is_rampe: bool,
    is_release: bool,
}

impl MidiIn {
    pub fn new(def: &SirenDef) -> MidiIn {
        let mut m = MidiIn {
            ratio: def.constants.engine_speed_frequency_ratio,
            change_volume_general: 1.0,
            note_on: 0.0,
            velocite: 0.0,
            pitch_bend: 0.0,
            control: [0.0; 127],
            control_final: 0.0,
            note_on_final: 0.0,
            volume_final: 0.0,
            tour_moteur: 0.0,
            var_vfo: 0.0,
            var_tremolo: 0.0,
            is_en_veille: false,
            vitesse: 0.0,
            tremolo: 0.0,
            velo_final: 0,
            vitesse_clapet: 0.0,
            ancien_velo: 0,
            count_create_attack: 0,
            constescursion: 10.0,
            incrementation_vibrato: 0.0,
            count_timer_audio: 0,
            is_attack_vibrato: false,
            is_rampe: false,
            is_release: false,
        };
        m.set_sample_rate(44100.0);
        m
    }

    pub fn set_sample_rate(&mut self, sr: f64) {
        self.incrementation_vibrato = ((512.0 / sr) / 0.025) as f32;
    }

    /// `tourMoteurFromNote()`: the engine speed a (fractional) MIDI note makes
    pub fn tour_moteur_from_note(&self, note: f32) -> f32 {
        let midi_note = note.max(0.0);
        let freq = 440.0f32 * 2.0f32.powf((midi_note - 81.0) / 12.0);
        if freq as f64 > 8.0 { freq * self.ratio } else { 0.0 }
    }

    /// `noteFromVitesse()`: the midicent an engine speed makes
    pub fn note_from_vitesse(&self, speed: f32) -> f32 {
        let mc = ((69.0f32 + 12.0f32 * ((speed / self.ratio) / 440.0f32).log2()) * 100.0f32).round() as i32;
        mc.max(0) as f32
    }

    /// the volume `realTimeStartNote` and CC 7 compute, clamped to [0, 500]
    fn volume_of(&self) -> f32 {
        let v = (self.velocite as f64 * (self.control[7] as f64 / 127.0) * (500.0 / 127.0)) as f32;
        v.clamp(0.0, 500.0)
    }

    pub fn control(&self, cc: usize) -> f32 {
        self.control.get(cc).copied().unwrap_or(0.0)
    }

    /// `timerAudio()`, every 512 samples
    pub fn timer_audio(&mut self, sink: &mut impl Sink) {
        self.send_varia(sink);
        if self.is_rampe {
            self.create_rampe(sink);
        }
        if self.is_release {
            self.create_release(sink);
        }
        if self.is_attack_vibrato && self.count_timer_audio == 0 {
            self.incremente_vibrato();
        }
        self.count_timer_audio += 1;
        if self.count_timer_audio >= 9 {
            self.count_timer_audio = 0;
        }
    }

    /// `realTimeStartNote()`
    pub fn start_note(&mut self, note: i32, velo: i32, sink: &mut impl Sink) {
        let c = &self.control;
        let vibrato_attack = c[1] != 0.0 && c[9] != 0.0 && c[11] != 0.0 && velo > 0;
        if vibrato_attack && (self.velocite == 0.0 || note as f32 != self.note_on) {
            self.control_final = 0.0;
            self.is_attack_vibrato = true;
        }
        self.note_on = note as f32;
        self.velocite = velo as f32;
        self.note_on_final = self.note_on + self.pitch_bend;
        self.volume_final = self.volume_of();
        self.tour_moteur = self.tour_moteur_from_note(self.note_on_final);
        self.send_varia(sink);
        self.send_vol((self.volume_final * self.change_volume_general) as i32, sink);
    }

    /// `realTimeStopNote()`
    pub fn stop_note(&mut self, note: i32, sink: &mut impl Sink) {
        if note as f32 == self.note_on {
            self.send_varia(sink);
            self.send_vol(0, sink);
            self.velocite = 0.0;
            self.volume_final = 0.0;
        }
    }

    /// `handleControlChange()`
    pub fn control_change(&mut self, cc: i32, value: i32, sink: &mut impl Sink) {
        let v = value as f32;
        match cc {
            121 => self.reset_sirene(),
            1 => {
                self.control[1] = v;
                if self.control[11] == 0.0 {
                    self.control_final = self.control[1];
                }
                if self.control[1] == 0.0 && self.is_attack_vibrato {
                    self.is_attack_vibrato = false;
                }
            }
            5 | 6 | 16 | 72 | 73 | 92 => self.control[cc as usize] = v,
            7 => {
                self.control[7] = v;
                self.volume_final = self.volume_of();
                self.send_vol((self.volume_final * self.change_volume_general) as i32, sink);
            }
            9 => {
                self.control[9] = v.clamp(0.0, 127.0);
                if self.control[9] == 0.0 && self.is_attack_vibrato {
                    self.is_attack_vibrato = false;
                }
            }
            11 => {
                self.control[11] = v;
                if self.control[11] == 0.0 && self.is_attack_vibrato {
                    self.is_attack_vibrato = false;
                }
            }
            15 => {
                self.control[15] = v;
                if value == 0 {
                    self.var_tremolo = 0.0;
                }
            }
            _ => {}
        }
    }

    /// `handlePitchWheel()`: takes effect at the next `timer_audio`
    pub fn pitch_wheel(&mut self, lsb: i32, msb: i32) {
        let raw = ((msb << 7) | lsb) as f32;
        self.pitch_bend = ((raw - 8192.0) as f64 / 8192.0) as f32;
        let range = self.control[16] as i32;
        self.note_on_final = self.note_on + self.pitch_bend * range as f32;
        self.tour_moteur = self.tour_moteur_from_note(self.note_on_final);
    }

    fn tremolo_of(&self, volume: i32) -> f32 {
        let v = volume as f64;
        let d = 256.0 / self.control[92] as f64;
        (v - ((v * (self.var_tremolo as f64 / 100.0).sin()) / d + v / d)) as f32
    }

    fn send_varia(&mut self, sink: &mut impl Sink) {
        let c1 = self.control[1];
        let c9 = self.control[9];
        let mut vibrato = 0.0f32;
        if self.var_vfo <= 628.0 && c9 != 0.0 && c1 != 0.0 {
            self.var_vfo += self.incrementation_vibrato * c9;
            vibrato = (((self.tour_moteur * self.constescursion * self.control_final) as f64 / 12700.0)
                * (self.var_vfo as f64 / 100.0).sin()) as f32;
        } else {
            self.var_vfo = 0.0;
        }
        let (c15, c92) = (self.control[15], self.control[92]);
        if self.var_tremolo <= 628.0 && c15 != 0.0 && c92 != 0.0 {
            self.var_tremolo += self.incrementation_vibrato * c15;
        } else {
            self.var_tremolo = 0.0;
        }
        if c15 != 0.0 && c92 != 0.0 && !self.is_release && !self.is_rampe && !self.is_en_veille {
            let volume = (self.volume_final * self.change_volume_general) as i32;
            self.tremolo = self.tremolo_of(volume);
            self.send_vol(volume, sink);
        }
        if self.control[5] == 0.0 {
            self.vitesse = self.tour_moteur;
        } else {
            let nbr = (((self.control[5] as f64 / 127.0) / 5.0) + 0.80) as f32;
            self.vitesse = ((1.0 - nbr as f64) * self.tour_moteur as f64 + (nbr * self.vitesse) as f64) as f32;
        }
        let note = self.note_from_vitesse(self.vitesse + vibrato);
        sink.note(note);
    }

    fn send_vol(&mut self, message: i32, sink: &mut impl Sink) {
        if self.control[73] > 0.0 && message >= 2 && self.ancien_velo <= 1 {
            // attack
            if self.is_rampe {
                self.is_rampe = false;
                self.count_create_attack -= 1;
            }
            self.is_release = false;
            self.count_create_attack += 1;
            if self.count_create_attack == 1 {
                self.is_rampe = true;
            } else {
                self.count_create_attack -= 1;
            }
        } else if self.control[72] > 0.0 && message <= 1 && self.ancien_velo >= 2 {
            // release
            if self.is_rampe {
                self.is_rampe = false;
                self.count_create_attack -= 1;
            }
            self.is_release = true;
        } else {
            if self.is_rampe && message <= 1 {
                self.is_rampe = false;
                self.count_create_attack -= 1;
            }
            if self.is_release && message > 1 {
                self.is_release = false;
            }
            if !self.is_rampe && !self.is_release {
                self.velo_final = if self.control[15] > 0.0 && self.control[92] > 0.0 { self.tremolo as i32 } else { message };
                self.vitesse_clapet = self.velo_final as f32;
                sink.volume(self.velo_final as f32);
            }
        }
        self.ancien_velo = message;
    }

    fn create_rampe(&mut self, sink: &mut impl Sink) {
        let vitesse_voulue = (self.volume_final * self.change_volume_general) as i32;
        let nbr = ((128.0 - self.control[73]) as f64 / 7.62) as f32;
        self.vitesse_clapet += nbr;
        let mut around = self.vitesse_clapet as i32;
        if around >= vitesse_voulue {
            if self.is_rampe {
                self.is_rampe = false;
                self.count_create_attack -= 1;
            }
            around = vitesse_voulue;
            self.vitesse_clapet = around as f32;
            self.velo_final = self.vitesse_clapet as i32;
        }
        if self.control[15] != 0.0 && self.control[92] != 0.0 {
            self.tremolo = self.tremolo_of(around);
            self.velo_final = self.tremolo as i32;
        } else {
            self.velo_final = around;
        }
        sink.volume(self.velo_final as f32);
    }

    fn create_release(&mut self, sink: &mut impl Sink) {
        let mut nbr = 128.0 - self.control[72];
        let vc = self.vitesse_clapet;
        if vc >= 250.0 {
            nbr = (nbr as f64 / 7.62) as f32;
        } else if vc >= 200.0 {
            nbr = (nbr as f64 / 10.0) as f32;
        } else if vc >= 150.0 {
            nbr = (nbr as f64 / 15.0) as f32;
        } else if vc >= 100.0 {
            nbr /= 20.0; // `nbr /= 20`: an int, so in float
        } else if vc >= 50.0 {
            nbr = (nbr as f64 / 25.0) as f32;
        } else if vc < 50.0 {
            nbr = (nbr as f64 / 30.0) as f32;
        }
        self.vitesse_clapet -= nbr;
        let around = self.vitesse_clapet as i32;
        if around <= 1 {
            self.is_release = false;
            self.velo_final = 0;
            self.tremolo = 0.0;
        } else if self.control[15] != 0.0 && self.control[92] != 0.0 {
            self.tremolo = self.tremolo_of(around);
            self.velo_final = self.tremolo as i32;
        } else {
            self.velo_final = around;
        }
        sink.volume(self.velo_final as f32);
    }

    fn incremente_vibrato(&mut self) {
        if self.control_final < self.control[1] {
            self.control_final = (self.control_final as f64 + self.control[11] as f64 / 12.7) as f32;
        } else {
            self.control_final = self.control[1];
            self.is_attack_vibrato = false;
        }
    }

    fn clear_motion(&mut self) {
        self.note_on_final = 0.0;
        self.note_on = 0.0;
        self.velocite = 0.0;
        self.tour_moteur = 0.0;
        self.var_vfo = 0.0;
        self.var_tremolo = 0.0;
        self.vitesse = 0.0;
        self.tremolo = 0.0;
        self.is_attack_vibrato = false;
        if self.is_rampe {
            self.is_rampe = false;
            self.count_create_attack -= 1;
        }
        self.is_release = false;
        self.velo_final = 500;
        self.volume_final = 500.0;
        self.vitesse_clapet = 0.0;
        self.ancien_velo = -1;
    }

    /// `stopSirene()`
    pub fn stop_sirene(&mut self) {
        self.clear_motion();
    }

    /// `resetSirene()`, also CC 121: controllers back to the firmware's defaults
    pub fn reset_sirene(&mut self) {
        for cc in [1, 5, 9, 11, 15, 18, 20, 21, 22, 23, 92, 72, 73] {
            self.control[cc] = 0.0;
        }
        self.control[17] = 100.0;
        self.control[6] = 64.0;
        self.control[12] = 127.0;
        self.control[13] = 127.0;
        self.control[7] = 127.0;
        self.pitch_bend = 0.0;
        self.clear_motion();
    }
}
