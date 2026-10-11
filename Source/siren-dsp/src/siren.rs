//! The seven sirens, as `sirenProperties.h` defines them: which data files
//! each one reads, and the constants of `Sirene` and `MidiIn`.

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum SirenId {
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
}

/// `sirenCategorySynthConstants`
#[derive(Clone, Copy, Debug)]
pub struct SynthConstants {
    pub note_min: i32,
    pub note_midi_cent_max: i32,
    pub pourcent_clapet_off: i32,
    pub coeff_picolo: i32,
    pub engine_speed_frequency_ratio: f32,
}

#[derive(Clone, Copy, Debug)]
pub struct SirenDef {
    pub id: SirenId,
    pub title: &'static str,
    /// suffix of `dataAmp`, `dataFreq` and `datadureTabs`
    pub data: &'static str,
    /// suffix of `dataVectorInterval`
    pub vector_interval: &'static str,
    pub constants: SynthConstants,
    /// `SirenVoiceUnit::maxNote`: the highest MIDI note, the last velocity
    /// range's top (`minMaxNoteClipped`) + 12.
    pub max_midi_note: i32,
}

const fn constants(
    note_min: i32,
    note_midi_cent_max: i32,
    pourcent_clapet_off: i32,
    coeff_picolo: i32,
    engine_speed_frequency_ratio: f32,
) -> SynthConstants {
    SynthConstants {
        note_min,
        note_midi_cent_max,
        pourcent_clapet_off,
        coeff_picolo,
        engine_speed_frequency_ratio,
    }
}

const ALTO: SynthConstants = constants(24, 7200, 7, 1, 5.0);
const BASS: SynthConstants = constants(24, 6400, 7, 1, 7.5);
const TENOR: SynthConstants = constants(24, 6500, 15, 1, 20.0 / 3.0);
const SOPRANO: SynthConstants = constants(36, 7900, 7, 1, 7.5);
const PICCOLO: SynthConstants = constants(36, 7900, 7, 2, 7.5);

pub const SIRENS: [SirenDef; 7] = [
    SirenDef {
        id: SirenId::S1,
        title: "Alto 1",
        data: "S1",
        vector_interval: "S1",
        constants: ALTO,
        max_midi_note: 72 + 12,
    },
    SirenDef {
        id: SirenId::S2,
        title: "Alto 2",
        data: "S1",
        // there is a dataVectorIntervalS2, but the plugin reads S1's
        vector_interval: "S1",
        constants: ALTO,
        max_midi_note: 72 + 12,
    },
    SirenDef {
        id: SirenId::S3,
        title: "Bass",
        data: "S3",
        vector_interval: "S3",
        constants: BASS,
        max_midi_note: 64 + 12,
    },
    SirenDef {
        id: SirenId::S4,
        title: "Tenor",
        data: "S4",
        vector_interval: "S4",
        constants: TENOR,
        max_midi_note: 65 + 12,
    },
    SirenDef {
        id: SirenId::S5,
        title: "Soprano 1",
        data: "S5",
        vector_interval: "S5",
        constants: SOPRANO,
        max_midi_note: 78 + 12,
    },
    SirenDef {
        id: SirenId::S6,
        title: "Soprano 2",
        data: "S5",
        vector_interval: "S5",
        constants: SOPRANO,
        max_midi_note: 78 + 12,
    },
    SirenDef {
        id: SirenId::S7,
        title: "Piccolo",
        data: "S7",
        // S7 reads S5's vectorInterval
        vector_interval: "S5",
        constants: PICCOLO,
        max_midi_note: 78 + 12,
    },
];

impl SirenId {
    pub fn parse(s: &str) -> Option<SirenId> {
        SIRENS.iter().find(|d| d.name() == s).map(|d| d.id)
    }

    pub fn def(self) -> &'static SirenDef {
        &SIRENS[self as usize]
    }
}

impl SirenDef {
    pub fn name(&self) -> &'static str {
        match self.id {
            SirenId::S1 => "S1",
            SirenId::S2 => "S2",
            SirenId::S3 => "S3",
            SirenId::S4 => "S4",
            SirenId::S5 => "S5",
            SirenId::S6 => "S6",
            SirenId::S7 => "S7",
        }
    }
}
