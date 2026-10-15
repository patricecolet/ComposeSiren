//! Clicks across a held note, for every Rust variant with frames.skip_silent on: the peak
//! of the second difference's RMS over 128-sample windows, against the median window, and
//! the frame playing at that peak (a click left by something other than a silent frame
//! shows up here: the loop's wrap at fmax, a frame boundary, a bank change).

use siren_dsp::Library;
use siren_dsp::settings::Engine;
use siren_dsp::siren::SirenId;
use siren_dsp::variants::VARIANTS;

fn peak(x: &[f32], from: usize) -> (f64, usize) {
    let d: Vec<f32> = x.windows(3).map(|w| w[2] - 2.0 * w[1] + w[0]).collect();
    let w = 128;
    let rms: Vec<f64> = d[from..]
        .chunks_exact(w)
        .map(|c| (c.iter().map(|&s| (s as f64).powi(2)).sum::<f64>() / w as f64).sqrt())
        .collect();
    let (i, max) = rms.iter().copied().enumerate().fold((0, 0.0), |a, (i, v)| if v > a.1 { (i, v) } else { a });
    let mut s = rms.clone();
    s.sort_by(f64::total_cmp);
    (20.0 * (max / s[s.len() / 2]).log10(), from + i * w + 1)
}

#[test]
#[ignore = "a scan: cargo test --release --test click_scan -- --ignored --nocapture"]
fn scan_s6_f_notes() {
    let lib = Library::new(siren_dsp::default_resources());
    let data = lib.data(SirenId::S6.def()).unwrap();
    let notes: Vec<usize> = std::env::var("NOTES").ok()
        .map(|s| s.split(',').map(|n| n.parse().unwrap()).collect())
        .unwrap_or_else(|| (5..80).step_by(12).filter(|&n| data.info[n].frames > 0).collect());
    let vars: Vec<String> = std::env::var("VARIANTS").ok().map(|s| s.split(',').map(String::from).collect())
        .unwrap_or_else(|| VARIANTS.iter().filter(|v| v.engine == Engine::Rust).map(|v| v.id.to_string()).collect());
    for &note in &notes {
        let info = &data.info[note];
        let seconds: f64 = std::env::var("SECONDS_").ok().map_or(8.0, |s| s.parse().unwrap());
        for vid in &vars {
            let var = siren_dsp::settings::variant(vid).unwrap();
            let mut st = var.dsp_settings().unwrap();
            st.skip_silent = std::env::var("SKIP").map_or(true, |s| s != "0");
            let mut v = lib.voice(SirenId::S6, 44100.0).unwrap();
            v.siren.set_timing(st.timing);
            st.apply_synthesis(&mut v.siren);
            v.glide = st.glide;
            v.note_on(note as i32 + 12, 127);
            let n = (seconds * 44100.0) as usize;
            let mut out = Vec::with_capacity(n);
            let mut fr = Vec::with_capacity(n);
            for _ in 0..n { out.push(v.process()); fr.push(v.siren.frame_of(note)); }
            let (db, at) = peak(&out, 22050);
            let maxf = fr.iter().copied().max().unwrap_or(0);
            eprintln!("S6 n{note:<3} fmax {:<4} data {:<4} {vid:<18} peak {db:5.1} dB at {:.3} s, frame {} (max frame reached {maxf}){}",
                info.fmax, info.data_frames, at as f64 / 44100.0, fr[at],
                if fr[at] + 2 >= info.fmax || fr[at] <= 1 { "  (at the loop wrap)" } else { "" });
        }
    }
}
