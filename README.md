# SIMD Image Processing Optimisation Experiment

This is a simple experimentation of SIMD instructions using intrinsics in C.

Instructions:

```bash
git clone --recurse-submodules git@github.com:Taahaa-M/image_simd_optim

cd image_simd_optim

make release

./image_process <filename.ppm>
```

To investigate the SIMD optimisation effect, use a profiler to find time deltas for the **_SIMD** suffixed functions and their counterparts.
For example, on Linux:
```bash
perf record ./image_process <filename.ppm>
perf report
```

To clean generated files: `make clean`
