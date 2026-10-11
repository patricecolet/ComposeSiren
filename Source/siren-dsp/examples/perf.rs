//! The Rust voices against the plugin's C++ voice: time per sample, data memory and load time.
//! Release, same machine, one voice at a time, a held note with vibrato, rendered in host-sized
//! blocks; each figure is the median of `RUNS` runs of `SECONDS` s of audio after a warm-up.
//!
//! ```text
//! SIREN_ORACLE_OPT=3 cargo run --release --example perf            # the tables (markdown)
//! SIREN_ORACLE_OPT=3 cargo run --release --example perf -- quick   # fewer runs
//! cargo run --release --example perf -- rss <cpp|raw|compact> [dir] # one load, its RSS
//! ```
//! The C++ is the plugin's own sources (`siren-dsp-oracle`, built like the plugin's Release with
//! `SIREN_ORACLE_OPT=3`); `compact` needs a folder of `.sirendata` (`ResourcesProcessing/compact.fsx`).

use std::hint::black_box;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Instant;

use siren_dsp::Library;
use siren_dsp::settings::{Engine, variant};
use siren_dsp::siren::SirenId;
use siren_dsp::variants::VARIANTS;
use siren_dsp_oracle::CppVoice;

const WARMUP: f64 = 0.5;

#[derive(Clone, Copy)]
struct Case {
    siren: SirenId,
    rate: f64,
    partials: i32,
    block: usize,
}

fn note_for(id: SirenId) -> i32 {
    // within every siren's range
    let d = id.def();
    d.constants.note_min + 12 + 19 // a fifth above its first octave
}

trait Engine1 {
    fn note_on(&mut self, n: i32, v: i32);
    fn cc(&mut self, c: i32, v: i32);
    fn block(&mut self, out: &mut [f32]);
}
impl Engine1 for CppVoice {
    fn note_on(&mut self, n: i32, v: i32) { CppVoice::note_on(self, n, v) }
    fn cc(&mut self, c: i32, v: i32) { self.control_change(c, v) }
    fn block(&mut self, out: &mut [f32]) { self.render(out) }
}
impl Engine1 for siren_dsp::sirene::Voice {
    fn note_on(&mut self, n: i32, v: i32) { siren_dsp::sirene::Voice::note_on(self, n, v) }
    fn cc(&mut self, c: i32, v: i32) { self.control_change(c, v) }
    fn block(&mut self, out: &mut [f32]) {
        for x in out.iter_mut() {
            *x = self.process();
        }
    }
}

/// ns per output sample of one run
fn run(e: &mut dyn Engine1, c: Case, seconds: f64) -> f64 {
    let mut buf = vec![0.0f32; c.block];
    e.note_on(note_for(c.siren), 100);
    e.cc(1, 70); // vibrato: the voice's whole path
    let blocks = |s: f64| ((s * c.rate) as usize).div_ceil(c.block);
    for _ in 0..blocks(WARMUP) {
        e.block(&mut buf);
    }
    let n = blocks(seconds);
    let t = Instant::now();
    for _ in 0..n {
        e.block(&mut buf);
        black_box(&buf);
    }
    t.elapsed().as_nanos() as f64 / (n * c.block) as f64
}

fn median(mut xs: Vec<f64>) -> f64 {
    xs.sort_by(f64::total_cmp);
    xs[xs.len() / 2]
}

fn engine(lib: &Library, res: &Path, id: &str, c: Case) -> Box<dyn Engine1> {
    if id == "cpp" {
        let mut v = CppVoice::new(res, &format!("{:?}", c.siren), c.rate).expect("C++ voice");
        v.set_quality(c.partials);
        Box::new(v)
    } else {
        let mut v = lib.voice(c.siren, c.rate).unwrap();
        variant(id).unwrap().dsp_settings().unwrap().apply(&mut v);
        v.siren.change_qualite(c.partials);
        Box::new(v)
    }
}

fn ids() -> Vec<&'static str> {
    VARIANTS.iter().map(|v| v.id).filter(|id| *id == "cpp" || variant(id).unwrap().engine == Engine::Rust).collect()
}

fn table(lib: &Library, res: &Path, title: &str, cases: &[(String, Case)], runs: usize, seconds: f64) {
    let ids = ids();
    println!("\n### {title}\n");
    println!("ns per sample (real-time factor: voices one core could play), median of {runs} runs of {seconds} s.\n");
    println!("| case | {} |", ids.join(" | "));
    println!("|---|{}|", ids.iter().map(|_| "---:").collect::<Vec<_>>().join("|"));
    for (label, c) in cases {
        let mut row = Vec::new();
        for id in &ids {
            let ns = median((0..runs).map(|_| run(engine(lib, res, id, *c).as_mut(), *c, seconds)).collect());
            row.push(format!("{ns:.0} ({:.0}x)", 1e9 / c.rate / ns));
        }
        println!("| {label} | {} |", row.join(" | "));
    }
}

fn rss_kib() -> u64 {
    let out = Command::new("ps").args(["-o", "rss=", "-p", &std::process::id().to_string()]).output().unwrap();
    String::from_utf8_lossy(&out.stdout).trim().parse().unwrap_or(0)
}

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let res = siren_dsp::default_resources();
    if args.first().map(String::as_str) == Some("rss") {
        let before = rss_kib();
        let t = Instant::now();
        let mut keep: Vec<Box<dyn std::any::Any>> = Vec::new();
        match args[1].as_str() {
            "cpp" => {
                for s in siren_dsp::siren::SIRENS {
                    keep.push(Box::new(CppVoice::new(&res, &format!("{:?}", s.id), 48000.0).unwrap()));
                }
            }
            kind => {
                let dir = args.get(2).map(PathBuf::from).unwrap_or(res.clone());
                let lib = Library::new(dir.clone());
                if kind == "compact" {
                    for name in siren_data::DATA_SETS {
                        let set = siren_data::DataSet::load_compact(&dir, name, name).unwrap();
                        lib.put_data(name, std::sync::Arc::new(siren_dsp::data::DataSet::new(set)));
                    }
                }
                for s in siren_dsp::siren::SIRENS {
                    keep.push(Box::new(lib.voice(s.id, 48000.0).unwrap()));
                }
                keep.push(Box::new(lib));
            }
        }
        let ms = t.elapsed().as_secs_f64() * 1e3;
        println!("{} {:.0} ms {} MiB", args[1], ms, (rss_kib() - before) / 1024);
        black_box(keep);
        return;
    }
    let quick = args.first().map(String::as_str) == Some("quick");
    let (runs, seconds) = if quick { (3, 1.0) } else { (7, 2.0) };
    let lib = Library::new(res.clone());
    let base = Case { siren: SirenId::S3, rate: 48000.0, partials: 30, block: 256 };
    let sirens: Vec<_> = siren_dsp::siren::SIRENS
        .iter()
        .map(|s| (format!("{:?}", s.id), Case { siren: s.id, ..base }))
        .collect();
    table(&lib, &res, "Each siren (48 kHz, 30 partials, 256-sample blocks)", &sirens, runs, seconds);
    let rates: Vec<_> = [44100.0, 48000.0, 96000.0].iter().map(|&r| (format!("{} kHz", r / 1000.0), Case { rate: r, ..base })).collect();
    table(&lib, &res, "Sample rates (S3, 30 partials, 256-sample blocks)", &rates, runs, seconds);
    let quals: Vec<_> = [30, 120, 200].iter().map(|&p| (format!("{p} partials"), Case { partials: p, ..base })).collect();
    table(&lib, &res, "Quality (S3, 48 kHz, 256-sample blocks)", &quals, runs, seconds);
    let blocks: Vec<_> = [64, 128, 256, 512].iter().map(|&b| (format!("{b} samples"), Case { block: b, ..base })).collect();
    table(&lib, &res, "Block size (S3, 48 kHz, 30 partials)", &blocks, runs, seconds);
}
