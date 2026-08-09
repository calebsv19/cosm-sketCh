# drawing_program (`sketCh`)

Internal program key/directory: `drawing_program`  
Branded program name: `sketCh`

This is the new hybrid drawing/icon creation program scaffold lane.

The Clang desktop build now defaults to managed Vulkan presentation through
vendored `vk_runtime 0.6.0` and `vk_renderer 1.3.1`. Drawing semantics remain
CPU/SDL-rasterized into an app-local compatibility canvas, then uploaded with
nearest filtering at the native drawable extent. Use `--render-backend
sdl-debug` as the explicit SDL oracle/fallback. This is presentation adoption,
not Vulkan compute adoption.

Identity lock (P1-S1):
- program key: `drawing_program`
- display/product name: `sketCh`
- release bundle id: `com.cosm.sketch`
- release program key: `drawing_program`
- launcher binary name: `sketch-launcher`
- app binary name: `drawing-program-bin`

Important distinction:
- `drawing_program` (`sketCh`) is the new program lane.
- `line_drawing` is an existing separate program and is not this lane.
