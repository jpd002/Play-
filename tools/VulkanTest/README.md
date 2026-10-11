# Vulkan GPU regression tests

Enable `BUILD_TESTS` and `BUILD_VULKAN_TESTS`, build `VulkanTest`, and run:

```
ctest --test-dir <build-directory> -C Release -R VulkanTest --output-on-failure
```

A desktop Vulkan device compatible with Play!'s renderer is required. The suite
checks x1, x2, and x4 when supported: color/depth sample coverage, subpixel triangle
edges, framebuffer textures, host/local transfers (32/24/16/8/4-bit), live resolution
changes, and invalid configuration values. On Windows it also exercises presentation
to a hidden test window. Configuration and logs stay in the test working directory.

Sprite regressions also check linear texture borders with CLAMP and REPEAT against
a CPU interpolation reference, including negative half-texel coordinates.

The opt-in arcade alignment regression checks that half-pixel right edges are
extended only above native resolution, that U is extended with X, and that the
starting edge remains fixed. Transposed sprites verify that bottom edges remain
unchanged. Final presentation now linearly interpolates
the scaled framebuffer samples, as OpenGL does, instead of nearest magnification.
Presentation shader execution is exercised for all supported framebuffer formats;
the suite does not compare swapchain pixels against a CPU reference.

The presentation aspect test checks that HDTV 480p is displayed at 4:3 while
its emulated resolution remains 720x480, NTSC keeps its existing aspect, and the
explicit widescreen preference still takes precedence.

Tests fail if the device cannot render at x2. They are opt-in so builds and test
runs without a GPU can continue to use the other test suites.

The native byte-conversion regression uses an artificial PSMT8 texture and an
identity palette. Each scaled memory plane contains a distinct pattern. It checks
that a 1.5x V slope selects the same native byte for every subpixel while preserving
the destination's sample plane. A disabled-alignment control reproduces incorrect
byte selection at x2/x4. This covers the small masked scratch-buffer passes used
by Taiko's channel conversion without including game data in the repository.
