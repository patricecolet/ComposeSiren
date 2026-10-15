//! The whole data set, note by note, for what makes a held note click:
//! - silent frames (the `frames.skip_silent` rule);
//! - slot jumps: frames where the partial slots stop following the same partials (a slot's
//!   frequency moves by more than 10% from the previous frame, weighted by amplitude), the
//!   loop's wrap (fmax-1 -> 0) included; a slot's oscillator then jumps, every loop;
//! - and a render (8 s held, 127) per variant: the high-passed residual's peak against its
//!   ±20 ms context (click_hp's metric), worst first.
//! `cargo test --release --test dataset_scan -- --ignored --nocapture` (VARIANTS=a,b to choose)

use siren_dsp::Library;
use siren_dsp::siren::SirenId;

#[path = "click_hp.rs"]
#[allow(dead_code)]
mod click_hp;

/// share of amplitude, between frames a and b of `note`, in slots whose frequency moves > 10%
pub fn slot_jump(d: &siren_dsp::data::DataSet, note: usize, a: usize, b: usize) -> f64 {
    let (fa, aa, fb, ab) = (d.freq(note, a), d.amp(note, a), d.freq(note, b), d.amp(note, b));
    let (mut moved, mut all) = (0.0, 0.0);
    for k in 0..fa.len() {
        let w = (aa[k] + ab[k]) as f64;
        if w <= 0.0 { continue; }
        all += w;
        let (x, y) = (fa[k] as f64, fb[k] as f64);
        if x <= 0.0 || y <= 0.0 || (x - y).abs() > 0.1 * x.max(y) { moved += w; }
    }
    if all > 0.0 { moved / all } else { 0.0 }
}

#[test]
#[ignore = "a scan of every note"]
fn scan() {
    let lib = Library::new(siren_dsp::default_resources());
    let vars: Vec<String> = std::env::var("VARIANTS").ok().map(|s| s.split(',').map(String::from).collect())
        .unwrap_or_else(|| vec!["trio-nearest-f32".into(), "frames".into(), "rust-exact".into()]);
    let skip = std::env::var("SKIP").map_or(true, |s| s != "0");
    let mut rows = Vec::new();
    for id in [SirenId::S1, SirenId::S3, SirenId::S4, SirenId::S5, SirenId::S7] {
        let d = lib.data(id.def()).unwrap();
        for n in 0..d.info.len() {
            let i = &d.info[n];
            if i.fmax < 2 || i.data_frames == 0 { continue; }
            let lim = i.fmax.min(i.frames);
            let repaired = (0..lim).filter(|&f| i.skip_to[f] as usize >= 1000).count();
            let silent = (0..lim).filter(|&f| (i.skip_to[f] as usize) < 1000 && i.skip_to[f] as usize != f).count();
            let mut jumps = Vec::new();
            for f in 0..i.fmax.min(i.frames) {
                let next = if f + 1 >= i.fmax { 0 } else { f + 1 };
                let j = slot_jump(&d, n, d.play_frame(n, f, skip), d.play_frame(n, next, skip));
                if j > 0.2 { jumps.push((f, next, j)); }
            }
            let mut worst = Vec::new();
            for v in vars.iter().filter(|v| v.as_str() != "none") {
                let (x, fr) = click_hp::render(id, v, n, skip, 3.0);
                let p = click_hp::hp_peaks(&x, 22050);
                let (db, at) = p.first().copied().unwrap_or((0.0, 0));
                worst.push((v.clone(), db, fr.get(at).copied().unwrap_or(0)));
            }
            let maxdb = if worst.is_empty() { silent as f64 } else { worst.iter().map(|w| w.1).fold(f64::MIN, f64::max) };
            rows.push((maxdb, format!("{id:?} n{n:<3} (sounds MIDI {:<3}) fmax {:<4} skipped {silent:<2} realigned {repaired:<3} jumps {:<3} {} | {}",
                n + 12, i.fmax, jumps.len(),
                jumps.iter().take(3).map(|(a, b, j)| format!("{a}->{b} {:.0}%", j * 100.0)).collect::<Vec<_>>().join(" "),
                worst.iter().map(|(v, db, f)| format!("{v} {db:.1} dB @f{f}")).collect::<Vec<_>>().join(", "))));
        }
    }
    rows.sort_by(|a, b| b.0.total_cmp(&a.0));
    let top: usize = std::env::var("TOP").map_or(40, |s| s.parse().unwrap());
    for (_, r) in rows.iter().take(top) { eprintln!("{r}"); }
    if let Ok(q) = std::env::var("ONLY") { for (_, r) in &rows { if q.split(',').any(|k| r.starts_with(k)) { eprintln!("* {r}"); } } }
}
