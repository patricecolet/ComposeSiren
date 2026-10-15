//! A finer click scan than click_scan's: the high-passed residual (third difference) in
//! 32-sample windows, each against the median of the windows around it (±20 ms), so a short
//! burst stands out even where the note's level moves. The top peaks, the frame playing
//! there and its time in the loop. Writes the render to $WAV_OUT (16-bit mono) when set.
use siren_dsp::Library;
use siren_dsp::siren::SirenId;

pub fn hp_peaks(x: &[f32], from: usize) -> Vec<(f64, usize)> {
    let w = 32;
    let d: Vec<f64> = x.windows(4).map(|v| (v[3] - 3.0 * v[2] + 3.0 * v[1] - v[0]) as f64).collect();
    let e: Vec<f64> = d[from..].chunks_exact(w).map(|c| (c.iter().map(|s| s * s).sum::<f64>() / w as f64).sqrt()).collect();
    let r = 28; // ±20 ms
    let mut out = Vec::new();
    for i in r..e.len().saturating_sub(r) {
        let mut ctx: Vec<f64> = e[i - r..i + r].iter().copied().collect();
        ctx.sort_by(f64::total_cmp);
        let m = ctx[ctx.len() / 2].max(1e-5);
        out.push((20.0 * (e[i] / m).log10(), from + i * w));
    }
    out.sort_by(|a, b| b.0.total_cmp(&a.0));
    out
}

fn wav(path: &str, x: &[f32]) {
    let mut b = Vec::new();
    let n = x.len() as u32 * 2;
    b.extend_from_slice(b"RIFF"); b.extend_from_slice(&(36 + n).to_le_bytes()); b.extend_from_slice(b"WAVEfmt ");
    b.extend_from_slice(&16u32.to_le_bytes()); b.extend_from_slice(&1u16.to_le_bytes()); b.extend_from_slice(&1u16.to_le_bytes());
    b.extend_from_slice(&44100u32.to_le_bytes()); b.extend_from_slice(&88200u32.to_le_bytes()); b.extend_from_slice(&2u16.to_le_bytes());
    b.extend_from_slice(&16u16.to_le_bytes()); b.extend_from_slice(b"data"); b.extend_from_slice(&n.to_le_bytes());
    let peak = x.iter().fold(0f32, |a, &s| a.max(s.abs())).max(1e-9);
    for &s in x { b.extend_from_slice(&((s / peak * 0.9 * 32767.0) as i16).to_le_bytes()); }
    std::fs::write(path, b).unwrap();
}

fn siren() -> SirenId { SirenId::parse(&std::env::var("SIREN").unwrap_or("S6".into())).unwrap() }

pub fn render(id: SirenId, variant: &str, note: usize, skip: bool, seconds: f64) -> (Vec<f32>, Vec<usize>) {
    let lib = Library::new(siren_dsp::default_resources());
    let st0 = siren_dsp::settings::variant(variant).unwrap();
    let mut st = st0.dsp_settings().unwrap();
    st.skip_silent = skip;
    let mut v = lib.voice(id, 44100.0).unwrap();
    v.siren.set_timing(st.timing);
    st.apply_synthesis(&mut v.siren);
    v.glide = std::env::var("GLIDE").is_ok_and(|g| g == "1") && st.glide;
    v.note_on(note as i32 + 12, 127);
    let n = (seconds * 44100.0) as usize;
    let mut out = Vec::with_capacity(n);
    let mut fr = Vec::with_capacity(n);
    for _ in 0..n { out.push(v.process()); fr.push(v.siren.frame_of(note)); }
    (out, fr)
}

#[test]
#[ignore = "a scan: VARIANT=.. NOTE=.. SKIP=0|1 cargo test --release --test click_hp -- --ignored --nocapture"]
fn scan() {
    let var = std::env::var("VARIANT").unwrap_or("trio-nearest-f32".into());
    let note: usize = std::env::var("NOTE").map_or(77, |s| s.parse().unwrap());
    let skip = std::env::var("SKIP").map_or(true, |s| s != "0");
    let (x, fr) = render(siren(), &var, note, skip, 8.0);
    if let Ok(p) = std::env::var("WAV_OUT") { wav(&p, &x); }
    let peaks = hp_peaks(&x, 44100);
    eprintln!("{var} n{note} skip {skip}:");
    for (db, at) in peaks.iter().take(12) {
        eprintln!("  {db:5.1} dB at {:.4} s, frame {}", *at as f64 / 44100.0, fr[*at]);
    }
}
