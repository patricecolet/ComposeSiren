//! Benchmarks `reverb-dsp` (`Mareverbe` and `Reverb` in `Plugin`, `StereoTime`, and C FFI modes)
//! against the plugin's C++ implementation (`siren-dsp-oracle`, built with `SIREN_ORACLE_OPT=3`).
//!
//! ```text
//! SIREN_ORACLE_OPT=3 cargo run --release --example perf
//! ```

use std::hint::black_box;
use std::time::Instant;

use reverb_dsp::ffi::{
    csr_reverb_begin_process_block, csr_reverb_enable, csr_reverb_free, csr_reverb_new,
    csr_reverb_process, csr_reverb_set_damping, csr_reverb_set_dry_wet, csr_reverb_set_high_cut,
    csr_reverb_set_low_cut, csr_reverb_set_room_size, csr_reverb_set_sample_rate,
    csr_reverb_set_width,
};
use reverb_dsp::{Mareverbe, Reverb, ReverbMode};
use siren_dsp_oracle::{CppMareverbe, CppReverb};

const WARMUP_SECONDS: f64 = 0.5;
const RUN_SECONDS: f64 = 4.0;
const RUNS: usize = 7;

fn median(mut xs: Vec<f64>) -> f64 {
    xs.sort_by(f64::total_cmp);
    xs[xs.len() / 2]
}

fn make_signal(len: usize) -> (Vec<f32>, Vec<f32>) {
    let mut l = Vec::with_capacity(len);
    let mut r = Vec::with_capacity(len);
    let mut state: u32 = 0x1234_5678;
    for i in 0..len {
        state = state.wrapping_mul(1664525).wrapping_add(1013904223);
        let noise_l = ((state >> 9) as f32 / (1u32 << 23) as f32) * 2.0 - 1.0;
        state = state.wrapping_mul(1664525).wrapping_add(1013904223);
        let noise_r = ((state >> 9) as f32 / (1u32 << 23) as f32) * 2.0 - 1.0;
        let t = i as f32 / 48000.0;
        l.push(0.4 * ((2.0 * std::f32::consts::PI * 220.0 * t).sin() + 0.25 * noise_l));
        r.push(0.4 * ((2.0 * std::f32::consts::PI * 330.0 * t).cos() + 0.25 * noise_r));
    }
    (l, r)
}

fn bench<F: FnMut(&[f32], &[f32], &mut [f32], &mut [f32])>(
    rate: f64,
    block: usize,
    mut f: F,
) -> f64 {
    let (in_l, in_r) = make_signal(block);
    let mut out_l = vec![0.0_f32; block];
    let mut out_r = vec![0.0_f32; block];
    let warmup_blocks = ((WARMUP_SECONDS * rate) as usize).div_ceil(block);
    let run_blocks = ((RUN_SECONDS * rate) as usize).div_ceil(block);

    for _ in 0..warmup_blocks {
        f(&in_l, &in_r, &mut out_l, &mut out_r);
        black_box((&out_l, &out_r));
    }

    let mut samples = Vec::with_capacity(RUNS);
    for _ in 0..RUNS {
        let t0 = Instant::now();
        for _ in 0..run_blocks {
            f(&in_l, &in_r, &mut out_l, &mut out_r);
            black_box((&out_l, &out_r));
        }
        let ns_per_sample = t0.elapsed().as_nanos() as f64 / (run_blocks * block) as f64;
        samples.push(ns_per_sample);
    }
    median(samples)
}

fn fmt_ns(ns: f64, rate: f64) -> String {
    let rtf = 1e9 / rate / ns;
    format!("{ns:.1} ns ({rtf:.0}x)")
}

fn main() {
    println!("### Reverb & Mareverbe performance (ns per stereo sample, median of {RUNS} runs of {RUN_SECONDS} s)\n");
    println!("| Rate / Block | C++ `Reverb` (in-place) | Rust `Reverb` (in-place) | Rust FFI `csr_reverb` (in-place) | C++ `Reverb` (out-of-place) | Rust `Reverb` (out-of-place) | Rust `StereoTime` (in-place) | C++ `mareverbe` | Rust `Mareverbe` |");
    println!("|---|---:|---:|---:|---:|---:|---:|---:|---:|");

    for &(rate, block) in &[
        (44100.0, 256),
        (48000.0, 64),
        (48000.0, 256),
        (48000.0, 512),
        (96000.0, 256),
    ] {
        // 1. C++ Reverb in-place
        let mut cpp_ip = CppReverb::new();
        cpp_ip.enable(true);
        cpp_ip.set_dry_wet(0.35);
        cpp_ip.set_damping(0.5);
        cpp_ip.set_room_size(0.75);
        cpp_ip.set_width(1.0);
        cpp_ip.set_low_cut(100.0);
        cpp_ip.set_high_cut(8000.0);
        cpp_ip.set_sample_rate(rate);
        let ns_cpp_ip = bench(rate, block, |il, ir, ol, or_| {
            ol.copy_from_slice(il);
            or_.copy_from_slice(ir);
            cpp_ip.begin_process_block();
            cpp_ip.process_in_place(ol, or_);
        });

        // 2. Rust Reverb in-place (Plugin mode)
        let mut rust_ip = Reverb::new();
        rust_ip.enable(true);
        rust_ip.set_dry_wet(0.35);
        rust_ip.set_damping(0.5);
        rust_ip.set_room_size(0.75);
        rust_ip.set_width(1.0);
        rust_ip.set_low_cut(100.0);
        rust_ip.set_high_cut(8000.0);
        rust_ip.set_sample_rate(rate);
        let ns_rust_ip = bench(rate, block, |il, ir, ol, or_| {
            ol.copy_from_slice(il);
            or_.copy_from_slice(ir);
            rust_ip.begin_process_block();
            rust_ip.process_in_place(ol, or_);
        });

        // 3. Rust FFI csr_reverb in-place
        let ffi_ip = unsafe {
            let r = csr_reverb_new();
            csr_reverb_enable(r, true);
            csr_reverb_set_dry_wet(r, 0.35);
            csr_reverb_set_damping(r, 0.5);
            csr_reverb_set_room_size(r, 0.75);
            csr_reverb_set_width(r, 1.0);
            csr_reverb_set_low_cut(r, 100.0);
            csr_reverb_set_high_cut(r, 8000.0);
            csr_reverb_set_sample_rate(r, rate);
            r
        };
        let ns_ffi_ip = bench(rate, block, |il, ir, ol, or_| unsafe {
            ol.copy_from_slice(il);
            or_.copy_from_slice(ir);
            csr_reverb_begin_process_block(ffi_ip);
            csr_reverb_process(
                ffi_ip,
                ol.as_ptr(),
                or_.as_ptr(),
                ol.as_mut_ptr(),
                or_.as_mut_ptr(),
                ol.len(),
            );
        });
        unsafe { csr_reverb_free(ffi_ip) };

        // 4. C++ Reverb out-of-place
        let mut cpp_oop = CppReverb::new();
        cpp_oop.enable(true);
        cpp_oop.set_dry_wet(0.35);
        cpp_oop.set_damping(0.5);
        cpp_oop.set_room_size(0.75);
        cpp_oop.set_width(1.0);
        cpp_oop.set_low_cut(100.0);
        cpp_oop.set_high_cut(8000.0);
        cpp_oop.set_sample_rate(rate);
        let ns_cpp_oop = bench(rate, block, |il, ir, ol, or_| {
            cpp_oop.begin_process_block();
            cpp_oop.process(il, ir, ol, or_);
        });

        // 5. Rust Reverb out-of-place
        let mut rust_oop = Reverb::new();
        rust_oop.enable(true);
        rust_oop.set_dry_wet(0.35);
        rust_oop.set_damping(0.5);
        rust_oop.set_room_size(0.75);
        rust_oop.set_width(1.0);
        rust_oop.set_low_cut(100.0);
        rust_oop.set_high_cut(8000.0);
        rust_oop.set_sample_rate(rate);
        let ns_rust_oop = bench(rate, block, |il, ir, ol, or_| {
            rust_oop.begin_process_block();
            rust_oop.process(il, ir, ol, or_);
        });

        // 6. Rust Reverb StereoTime in-place
        let mut rust_st = Reverb::new();
        rust_st.set_mode(ReverbMode::StereoTime);
        rust_st.enable(true);
        rust_st.set_dry_wet(0.35);
        rust_st.set_damping(0.5);
        rust_st.set_room_size(0.75);
        rust_st.set_width(1.0);
        rust_st.set_low_cut(100.0);
        rust_st.set_high_cut(8000.0);
        rust_st.set_sample_rate(rate);
        let ns_rust_st = bench(rate, block, |il, ir, ol, or_| {
            ol.copy_from_slice(il);
            or_.copy_from_slice(ir);
            rust_st.begin_process_block();
            rust_st.process_in_place(ol, or_);
        });

        // 7. C++ mareverbe::process_stereo
        let mut cpp_m = CppMareverbe::new();
        cpp_m.set_room_size(0.75);
        cpp_m.set_damp(0.5);
        cpp_m.set_wet(0.35);
        cpp_m.set_dry(0.65);
        cpp_m.set_width(1.0);
        let ns_cpp_m = bench(rate, block, |il, ir, ol, or_| {
            cpp_m.process_stereo(il, ir, ol, or_);
        });

        // 8. Rust Mareverbe::process_stereo
        let mut rust_m = Mareverbe::new();
        rust_m.set_room_size(0.75);
        rust_m.set_damp(0.5);
        rust_m.set_wet(0.35);
        rust_m.set_dry(0.65);
        rust_m.set_width(1.0);
        let ns_rust_m = bench(rate, block, |il, ir, ol, or_| {
            rust_m.process_stereo(il, ir, ol, or_);
        });

        let _ = csr_reverb_process;
        println!(
            "| {:.1} kHz / {block} | {} | {} | {} | {} | {} | {} | {} | {} |",
            rate / 1000.0,
            fmt_ns(ns_cpp_ip, rate),
            fmt_ns(ns_rust_ip, rate),
            fmt_ns(ns_ffi_ip, rate),
            fmt_ns(ns_cpp_oop, rate),
            fmt_ns(ns_rust_oop, rate),
            fmt_ns(ns_rust_st, rate),
            fmt_ns(ns_cpp_m, rate),
            fmt_ns(ns_rust_m, rate),
        );
    }
}
