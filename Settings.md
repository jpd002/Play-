# Settings

This is a description of the settings that are available for the emulator. The goal of this document is to have settings that are consistent on all supported platforms.

## General Settings

### Show Frame and Draw Call Counters

Enables display of "frames per second" (f/s) and "draw calls per frame" (dc/f) statistics on screen.

*(Android & iOS only)*

### Show Virtual Pad

Enables display of on-screen virtual controller pad.

*(Android & iOS only)*

## Video Settings

### Enable High Resolution Mode

Enables usage of 2x sized framebuffers for rendering.

The desktop OpenGL and Vulkan renderers support resolution multipliers. Vulkan
keeps separate color and depth samples for the higher resolution and limits the
effective multiplier to the device's buffer, framebuffer, point, and line limits.
The status bar shows the effective Vulkan multiplier. The Android Vulkan renderer
currently renders at native resolution.

### Force Bilinear Filtering

Forces usage of bilinear filtering on all textures.

## Audio Settings

### Enable Audio Output

Enables audio output.
