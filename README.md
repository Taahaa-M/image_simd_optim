This is a simple experimentation of SIMD instructions using intrinsics in C.

Instructions:

```git clone --recurse-submodules <your-repository-url>```
Change *$OS* in *Makefile* to reflect the Operating System used.

```make release```

```./image_process <filename.ppm>```

To investigate the SIMD optimisation effect, use a profiler to find time deltas for the _SIMD suffixed functions and their counterparts.

To clean:

```make clean```
