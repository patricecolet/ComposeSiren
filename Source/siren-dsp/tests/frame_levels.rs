use siren_dsp::Library;
use siren_dsp::siren::SirenId;

#[test]
#[ignore = "a dump: cargo test --release --test frame_levels -- --ignored --nocapture"]
fn s5_77_levels() {
    let note: usize = std::env::var("NOTE").map_or(77, |s| s.parse().unwrap());
    let (lo, hi): (usize, usize) = std::env::var("RANGE").ok().map_or((425, 456), |s| { let v: Vec<usize> = s.split("..").map(|x| x.parse().unwrap()).collect(); (v[0], v[1]) });
    let lib = Library::new(siren_dsp::default_resources());
    let d = lib.data(SirenId::parse(&std::env::var("SIREN").unwrap_or("S6".into())).unwrap().def()).unwrap();
    let i = &d.info[note];
    let mut e: Vec<f64> = i.frame_energy[..].iter().map(|&x| x as f64).collect();
    let raw = e.clone();
    e.sort_by(f64::total_cmp);
    let med = e[e.len() / 2];
    eprintln!("fdur {} fmax {} frames {} data_frames {} median {med:.4}", i.fdur, i.fmax, i.frames, i.data_frames);
    for f in lo..hi.min(i.frames) {
        let nb: f64 = [-3i32, -2, 2, 3].iter().map(|&k| raw.get((f as i32 + k).max(0) as usize).copied().unwrap_or(0.0)).sum::<f64>() / 4.0;
        let a = d.amp(note, f);
        let fr = d.freq(note, f);
        let top: Vec<String> = (0..4).map(|p| format!("{:.0}Hz {:.3}", fr[p], a[p])).collect();
        eprintln!("f{f} {:6.1} dB med {:6.1} dB nb  skip_to {}  {}", 20.0 * (raw[f] / med).log10(), 20.0 * (raw[f] / nb).log10(), i.skip_to[f], top.join(" | "));
    }
}
