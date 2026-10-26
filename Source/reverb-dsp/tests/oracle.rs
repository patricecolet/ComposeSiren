//! Golden tests comparing `reverb-dsp` (`Mareverbe`, `Reverb`, and `ffi`) against the
//! plugin's C++ implementation (`mareverbe.cpp`, `comb.cpp`, `allpass.cpp`, `Reverb.cpp`)
//! sample for sample (`a.to_bits() == b.to_bits()`).

use reverb_dsp::{Mareverbe, Reverb, ReverbMode, ffi};
use siren_dsp_oracle::{CppMareverbe, CppReverb};

fn make_stereo_signal(n: usize, amp: f32) -> (Vec<f32>, Vec<f32>) {
    let mut l = Vec::with_capacity(n);
    let mut r = Vec::with_capacity(n);
    for i in 0..n {
        let t = i as f32 / 44100.0;
        // Rich harmonic + transient test signal (asymmetric L/R)
        let burst = if (i % 4096) < 512 { 1.0 } else { 0.15 };
        let sig_l = ((t * 220.0 * std::f32::consts::TAU).sin() * 0.6
            + (t * 660.0 * std::f32::consts::TAU).cos() * 0.3
            + (t * 1760.0 * std::f32::consts::TAU).sin() * 0.2)
            * burst
            * amp;
        let sig_r = ((t * 330.0 * std::f32::consts::TAU).sin() * 0.5
            + (t * 990.0 * std::f32::consts::TAU).cos() * 0.35
            + (t * 2420.0 * std::f32::consts::TAU).sin() * 0.25)
            * burst
            * amp;
        l.push(sig_l);
        r.push(sig_r);
    }
    (l, r)
}

fn assert_bits_eq(label: &str, a: &[f32], b: &[f32]) {
    assert_eq!(a.len(), b.len(), "{label}: length mismatch");
    for (i, (&x, &y)) in a.iter().zip(b.iter()).enumerate() {
        if x.to_bits() != y.to_bits() {
            panic!(
                "{label} sample {i}: rust={x} ({:#010x}) != cpp={y} ({:#010x}), diff={}",
                x.to_bits(),
                y.to_bits(),
                (x - y).abs()
            );
        }
    }
}

#[test]
fn mareverbe_matches_cpp_bit_for_bit() {
    let mut rust = Mareverbe::new();
    let mut cpp = CppMareverbe::new();

    assert_eq!(rust.room_size().to_bits(), cpp.room_size().to_bits());
    assert_eq!(rust.damp().to_bits(), cpp.damp().to_bits());
    assert_eq!(rust.wet().to_bits(), cpp.wet().to_bits());
    assert_eq!(rust.dry().to_bits(), cpp.dry().to_bits());
    assert_eq!(rust.width().to_bits(), cpp.width().to_bits());
    assert_eq!(rust.mode().to_bits(), cpp.mode().to_bits());

    let configs: &[(f32, f32, f32, f32, f32, f32, f32)] = &[
        // (room_size, damp, wet, dry, width, mode, signal_amp)
        (0.5, 0.5, 1.0 / 3.0, 0.0, 1.0, 0.0, 1.0),
        (0.85, 0.2, 0.9, 0.25, 0.75, 0.0, 1.2),
        (0.95, 0.05, 1.0, 0.5, 0.3, 0.0, 8.0), // triggers dsp_clip(..., 1.0)
        (0.4, 0.8, 0.7, 0.1, 1.0, 0.6, 1.0),   // freeze mode >= 0.5
        (0.6, 0.4, 0.5, 0.3, 0.0, 0.0, 0.8),   // unfreeze, mono width
    ];

    let (in_l, in_r) = make_stereo_signal(4096, 1.0);
    for (idx, &(room, damp, wet, dry, width, mode, amp)) in configs.iter().enumerate() {
        rust.set_room_size(room);
        rust.set_damp(damp);
        rust.set_wet(wet);
        rust.set_dry(dry);
        rust.set_width(width);
        rust.set_mode(mode);

        cpp.set_room_size(room);
        cpp.set_damp(damp);
        cpp.set_wet(wet);
        cpp.set_dry(dry);
        cpp.set_width(width);
        cpp.set_mode(mode);

        let src_l: Vec<f32> = in_l.iter().map(|&x| x * amp).collect();
        let src_r: Vec<f32> = in_r.iter().map(|&x| x * amp).collect();

        let mut rust_l = vec![0.0_f32; src_l.len()];
        let mut rust_r = vec![0.0_f32; src_r.len()];
        let mut cpp_l = vec![0.0_f32; src_l.len()];
        let mut cpp_r = vec![0.0_f32; src_r.len()];

        rust.process_stereo(&src_l, &src_r, &mut rust_l, &mut rust_r);
        cpp.process_stereo(&src_l, &src_r, &mut cpp_l, &mut cpp_r);

        assert_bits_eq(&format!("mareverbe cfg {idx} L"), &rust_l, &cpp_l);
        assert_bits_eq(&format!("mareverbe cfg {idx} R"), &rust_r, &cpp_r);
    }

    // Test mute() and in-place processing
    rust.mute();
    cpp.mute();
    let (mut rust_l, mut rust_r) = make_stereo_signal(2048, 1.5);
    let mut cpp_l = rust_l.clone();
    let mut cpp_r = rust_r.clone();
    rust.process_stereo_in_place(&mut rust_l, &mut rust_r);
    cpp.process_stereo_in_place(&mut cpp_l, &mut cpp_r);
    assert_bits_eq("mareverbe in-place L", &rust_l, &cpp_l);
    assert_bits_eq("mareverbe in-place R", &rust_r, &cpp_r);
}

type BlockStep = (bool, f32, f32, f32, f32, f32, f32, usize);

#[test]
fn reverb_out_of_place_matches_cpp_bit_for_bit() {
    let (in_l, in_r) = make_stereo_signal(8192, 0.9);

    for &sr in &[22050.0, 44100.0, 48000.0, 88200.0, 96000.0] {
        let mut rust = Reverb::new();
        let mut cpp = CppReverb::new();

        rust.set_sample_rate(sr);
        cpp.set_sample_rate(sr);

        let block_steps: &[BlockStep] = &[
            // (enable, dry_wet, damping, room_size, width, low_cut, high_cut, block_len)
            (true, 0.35, 0.5, 0.5, 1.0, 100.0, 8000.0, 512),
            (true, 0.65, 0.25, 0.8, 0.85, 220.0, 6500.0, 256),
            (true, 1.0, 0.7, 0.9, 0.4, 80.0, 9500.0, 1024),
            (false, 0.5, 0.5, 0.6, 1.0, 150.0, 5000.0, 512),
            (true, 0.45, 0.3, 0.75, 0.9, 300.0, 4000.0, 512),
            (true, 0.8, 0.1, 0.95, 1.0, 60.0, 10000.0, 1280),
        ];

        let mut offset = 0;
        for (step_idx, &(en, dw, damp, room, width, lc, hc, len)) in block_steps.iter().enumerate()
        {
            rust.enable(en);
            rust.set_dry_wet(dw);
            rust.set_damping(damp);
            rust.set_room_size(room);
            rust.set_width(width);
            rust.set_low_cut(lc);
            rust.set_high_cut(hc);
            rust.begin_process_block();

            cpp.enable(en);
            cpp.set_dry_wet(dw);
            cpp.set_damping(damp);
            cpp.set_room_size(room);
            cpp.set_width(width);
            cpp.set_low_cut(lc);
            cpp.set_high_cut(hc);
            cpp.begin_process_block();

            let slice_l = &in_l[offset..offset + len];
            let slice_r = &in_r[offset..offset + len];
            let mut rust_l = vec![0.0_f32; len];
            let mut rust_r = vec![0.0_f32; len];
            let mut cpp_l = vec![0.0_f32; len];
            let mut cpp_r = vec![0.0_f32; len];

            rust.process(slice_l, slice_r, &mut rust_l, &mut rust_r);
            cpp.process(slice_l, slice_r, &mut cpp_l, &mut cpp_r);

            assert_bits_eq(&format!("reverb out-of-place sr={sr} step={step_idx} L"), &rust_l, &cpp_l);
            assert_bits_eq(&format!("reverb out-of-place sr={sr} step={step_idx} R"), &rust_r, &cpp_r);
            offset += len;
        }
    }
}

#[test]
fn reverb_in_place_matches_cpp_bit_for_bit() {
    let (in_l, in_r) = make_stereo_signal(8192, 0.9);

    for &sr in &[44100.0, 48000.0, 96000.0] {
        let mut rust = Reverb::new();
        let mut cpp = CppReverb::new();

        // Exact constructor + prepareToPlay order from SirenOrchestraPluginProcessor
        rust.enable(true);
        rust.set_dry_wet(0.0);
        rust.set_damping(0.5);
        rust.set_room_size(0.5);
        rust.set_width(1.0);
        rust.set_low_cut(100.0);
        rust.set_high_cut(8000.0);
        rust.set_sample_rate(sr);

        cpp.enable(true);
        cpp.set_dry_wet(0.0);
        cpp.set_damping(0.5);
        cpp.set_room_size(0.5);
        cpp.set_width(1.0);
        cpp.set_low_cut(100.0);
        cpp.set_high_cut(8000.0);
        cpp.set_sample_rate(sr);

        let block_steps: &[BlockStep] = &[
            (true, 0.0, 0.5, 0.5, 1.0, 100.0, 8000.0, 512),
            (true, 0.4, 0.35, 0.7, 0.9, 120.0, 7500.0, 512),
            (true, 0.75, 0.2, 0.85, 0.6, 250.0, 6000.0, 512),
            (false, 0.75, 0.2, 0.85, 0.6, 250.0, 6000.0, 512),
            (true, 0.5, 0.6, 0.6, 1.0, 90.0, 9000.0, 1024),
        ];

        let mut offset = 0;
        for (step_idx, &(en, dw, damp, room, width, lc, hc, len)) in block_steps.iter().enumerate()
        {
            rust.enable(en);
            rust.set_dry_wet(dw);
            rust.set_damping(damp);
            rust.set_room_size(room);
            rust.set_width(width);
            rust.set_low_cut(lc);
            rust.set_high_cut(hc);
            rust.begin_process_block();

            cpp.enable(en);
            cpp.set_dry_wet(dw);
            cpp.set_damping(damp);
            cpp.set_room_size(room);
            cpp.set_width(width);
            cpp.set_low_cut(lc);
            cpp.set_high_cut(hc);
            cpp.begin_process_block();

            let mut rust_l = in_l[offset..offset + len].to_vec();
            let mut rust_r = in_r[offset..offset + len].to_vec();
            let mut cpp_l = rust_l.clone();
            let mut cpp_r = rust_r.clone();

            rust.process_in_place(&mut rust_l, &mut rust_r);
            cpp.process_in_place(&mut cpp_l, &mut cpp_r);

            assert_bits_eq(&format!("reverb in-place sr={sr} step={step_idx} L"), &rust_l, &cpp_l);
            assert_bits_eq(&format!("reverb in-place sr={sr} step={step_idx} R"), &rust_r, &cpp_r);
            offset += len;
        }
    }
}

#[test]
fn reverb_parameter_ramps_across_blocks_match_cpp_bit_for_bit() {
    let (in_l, in_r) = make_stereo_signal(4096, 0.8);
    let mut rust = Reverb::new();
    let mut cpp = CppReverb::new();

    rust.set_low_cut(100.0);
    rust.set_high_cut(8000.0);
    rust.set_sample_rate(44100.0);

    cpp.set_low_cut(100.0);
    cpp.set_high_cut(8000.0);
    cpp.set_sample_rate(44100.0);

    let targets: &[(f32, f32, f32, f32)] = &[
        (0.75, 0.85, 0.25, 0.6),
        (0.15, 0.35, 0.9, 1.0),
        (0.95, 0.65, 0.1, 0.2),
        (0.0, 0.5, 0.5, 1.0),
    ];

    let sub_block = 128;
    let mut offset = 0;
    for (t_idx, &(dw, room, damp, width)) in targets.iter().enumerate() {
        rust.set_dry_wet(dw);
        rust.set_room_size(room);
        rust.set_damping(damp);
        rust.set_width(width);
        rust.begin_process_block();

        cpp.set_dry_wet(dw);
        cpp.set_room_size(room);
        cpp.set_damping(damp);
        cpp.set_width(width);
        cpp.begin_process_block();

        // Step multiple process() calls per begin_process_block() so the ramp advances
        for sub in 0..4 {
            let slice_l = &in_l[offset..offset + sub_block];
            let slice_r = &in_r[offset..offset + sub_block];
            let mut rust_l = vec![0.0_f32; sub_block];
            let mut rust_r = vec![0.0_f32; sub_block];
            let mut cpp_l = vec![0.0_f32; sub_block];
            let mut cpp_r = vec![0.0_f32; sub_block];

            rust.process(slice_l, slice_r, &mut rust_l, &mut rust_r);
            cpp.process(slice_l, slice_r, &mut cpp_l, &mut cpp_r);

            assert_bits_eq(
                &format!("reverb ramp target={t_idx} sub={sub} L"),
                &rust_l,
                &cpp_l,
            );
            assert_bits_eq(
                &format!("reverb ramp target={t_idx} sub={sub} R"),
                &rust_r,
                &cpp_r,
            );
            offset += sub_block;
        }
    }
}

#[test]
fn ffi_matches_cpp_bit_for_bit() {
    let (in_l, in_r) = make_stereo_signal(2048, 0.85);
    let mut cpp = CppReverb::new();
    cpp.set_sample_rate(48000.0);
    cpp.enable(true);
    cpp.set_dry_wet(0.55);
    cpp.set_damping(0.35);
    cpp.set_room_size(0.75);
    cpp.set_width(0.85);
    cpp.set_low_cut(150.0);
    cpp.set_high_cut(7500.0);
    cpp.begin_process_block();

    unsafe {
        let r = ffi::csr_reverb_new();
        assert!(!r.is_null());
        ffi::csr_reverb_set_sample_rate(r, 48000.0);
        ffi::csr_reverb_enable(r, true);
        ffi::csr_reverb_set_dry_wet(r, 0.55);
        ffi::csr_reverb_set_damping(r, 0.35);
        ffi::csr_reverb_set_room_size(r, 0.75);
        ffi::csr_reverb_set_width(r, 0.85);
        ffi::csr_reverb_set_low_cut(r, 150.0);
        ffi::csr_reverb_set_high_cut(r, 7500.0);
        ffi::csr_reverb_begin_process_block(r);

        // Out-of-place first half
        let mut ffi_l = vec![0.0_f32; 1024];
        let mut ffi_r = vec![0.0_f32; 1024];
        let mut cpp_l = vec![0.0_f32; 1024];
        let mut cpp_r = vec![0.0_f32; 1024];
        ffi::csr_reverb_process(
            r,
            in_l.as_ptr(),
            in_r.as_ptr(),
            ffi_l.as_mut_ptr(),
            ffi_r.as_mut_ptr(),
            1024,
        );
        cpp.process(&in_l[..1024], &in_r[..1024], &mut cpp_l, &mut cpp_r);
        assert_bits_eq("ffi out-of-place L", &ffi_l, &cpp_l);
        assert_bits_eq("ffi out-of-place R", &ffi_r, &cpp_r);

        // In-place second half
        let mut ffi_ip_l = in_l[1024..2048].to_vec();
        let mut ffi_ip_r = in_r[1024..2048].to_vec();
        let mut cpp_ip_l = ffi_ip_l.clone();
        let mut cpp_ip_r = ffi_ip_r.clone();
        ffi::csr_reverb_process(
            r,
            ffi_ip_l.as_ptr(),
            ffi_ip_r.as_ptr(),
            ffi_ip_l.as_mut_ptr(),
            ffi_ip_r.as_mut_ptr(),
            1024,
        );
        cpp.process_in_place(&mut cpp_ip_l, &mut cpp_ip_r);
        assert_bits_eq("ffi in-place L", &ffi_ip_l, &cpp_ip_l);
        assert_bits_eq("ffi in-place R", &ffi_ip_r, &cpp_ip_r);

        ffi::csr_reverb_free(r);
    }
}

#[test]
fn stereo_time_mode_preserves_dry_in_place_and_filters_independently() {
    let (in_l, in_r) = make_stereo_signal(2048, 0.7);

    let mut r_out = Reverb::with_plugin_defaults(48000.0);
    r_out.set_mode(ReverbMode::StereoTime);
    r_out.set_dry_wet(0.35);
    r_out.begin_process_block();

    let mut r_ip = r_out.clone();

    let mut out_l = vec![0.0_f32; in_l.len()];
    let mut out_r = vec![0.0_f32; in_r.len()];
    r_out.process(&in_l, &in_r, &mut out_l, &mut out_r);

    let mut ip_l = in_l.clone();
    let mut ip_r = in_r.clone();
    r_ip.process_in_place(&mut ip_l, &mut ip_r);

    // In StereoTime mode, in-place and out-of-place produce the exact same samples.
    assert_bits_eq("stereo_time in-place vs out-of-place L", &ip_l, &out_l);
    assert_bits_eq("stereo_time in-place vs out-of-place R", &ip_r, &out_r);
}
