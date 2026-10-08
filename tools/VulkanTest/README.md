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

Tests fail if the device cannot render at x2. They are opt-in so builds and test
runs without a GPU can continue to use the other test suites.
