import Play from "./Play";
import DiscImageDevice from "./DiscImageDevice";

export let PlayModule : any = null;

let module_overrides = {
    locateFile: function(path : string) {
        const baseURL = window.location.origin + window.location.pathname.substring(0, window.location.pathname.lastIndexOf( "/" ));
        return baseURL + '/' + path;
    },
    mainScriptUrlOrBlob: "",
};

export let initPlayModule = async function() {
    // Log SharedArrayBuffer availability for debugging (not required since we disabled pthreads)
    console.log('SharedArrayBuffer available:', typeof SharedArrayBuffer !== 'undefined');
    
    module_overrides.mainScriptUrlOrBlob = module_overrides.locateFile('Play.js');
    
    try {
        console.log('Loading Play WASM module...');
        PlayModule = await Play(module_overrides);
        console.log('Play WASM module loaded successfully');
        
        // Verify WASM memory is initialized
        if (!PlayModule) {
            throw new Error('PlayModule is null or undefined');
        }
        
        if (!PlayModule.HEAP8) {
            throw new Error('WASM module memory (HEAP8) not properly initialized');
        }
        
        console.log('WASM memory initialized, HEAP8 size:', PlayModule.HEAP8.length);
        
        console.log('Creating /work directory...');
        PlayModule.FS.mkdir("/work");
        
        console.log('Initializing DiscImageDevice...');
        PlayModule.discImageDevice = new DiscImageDevice(PlayModule);
        
        console.log('Calling initVm...');
        // Wait for module to be fully initialized
        if (!PlayModule.ready) {
            await PlayModule;
        }
        
        // Ensure canvas exists before calling initVm (initVm creates WebGL context on #outputCanvas)
        // Note: Do NOT create a WebGL context here - let initVm() create it via emscripten_webgl_create_context
        let canvas = document.getElementById('outputCanvas') as HTMLCanvasElement;
        if (!canvas) {
            console.warn('Canvas #outputCanvas not found, creating it...');
            canvas = document.createElement('canvas');
            canvas.id = 'outputCanvas';
            // Set minimum size for WebGL context creation (some browsers require non-zero size)
            // Reduced from 640x480 to 480x360 for low RAM device compatibility
            canvas.width = 480;
            canvas.height = 360;
            // Make canvas visible (some browsers require canvas to be in viewport for WebGL)
            // We'll hide it with CSS if needed, but keep it in the layout
            canvas.style.position = 'fixed';
            canvas.style.left = '0';
            canvas.style.top = '0';
            canvas.style.width = '480px';
            canvas.style.height = '360px';
            canvas.style.zIndex = '-1'; // Behind everything
            canvas.style.pointerEvents = 'none'; // Don't intercept mouse events
            document.body.appendChild(canvas);
            console.log('Canvas created with size:', canvas.width, 'x', canvas.height);
        } else {
            // Ensure canvas has valid dimensions
            if (canvas.width === 0 || canvas.height === 0) {
                canvas.width = 480;
                canvas.height = 360;
                console.log('Canvas dimensions updated to:', canvas.width, 'x', canvas.height);
            }
        }
        
        // Verify WebGL is available (but don't create a context - let initVm do that)
        const testCanvas = document.createElement('canvas');
        const testGl = testCanvas.getContext('webgl2') || testCanvas.getContext('webgl');
        if (!testGl) {
            throw new Error('WebGL is not supported in this environment. Cannot initialize Play! emulator.');
        }
        console.log('WebGL support verified, WebGL version:', testGl.getParameter(testGl.VERSION));
        
        // Verify canvas is findable by CSS selector (emscripten_webgl_create_context uses querySelector)
        const canvasBySelector = document.querySelector('#outputCanvas');
        if (!canvasBySelector) {
            throw new Error('Canvas #outputCanvas not found by CSS selector. This will cause emscripten_webgl_create_context to fail.');
        }
        if (canvasBySelector !== canvas) {
            console.warn('Warning: Multiple elements found with id "outputCanvas"');
        }
        console.log('Canvas #outputCanvas verified, accessible by CSS selector');
        
        // Try embind export first (initVm), then EXPORTED_FUNCTIONS (_initVm), then ccall
        let initResult;
        if (typeof PlayModule.initVm === 'function') {
            // Call via embind (preferred when using --bind)
            // initVm is void, but might throw on failure (e.g., assert failure in WebGL context creation)
            try {
                initResult = PlayModule.initVm();
                // initVm is void, so initResult should be undefined
                // If it returns a number, something went wrong
                if (typeof initResult === 'number') {
                    console.error('initVm returned unexpected numeric value:', initResult);
                    throw new Error(`initVm returned unexpected value: ${initResult}. This might indicate a WebGL context creation failure.`);
                }
            } catch (error: any) {
                // Log detailed error information
                console.error('initVm threw an error:', error);
                console.error('Error details:', {
                    type: typeof error,
                    value: error,
                    message: error?.message,
                    name: error?.name,
                    stack: error?.stack
                });
                
                // Handle numeric errors (Emscripten assertion failures often throw numbers)
                if (typeof error === 'number') {
                    // Convert to hex for debugging
                    const errorHex = '0x' + error.toString(16).toUpperCase();
                    console.error(`Numeric error code: ${error} (${errorHex})`);
                    
                    // Check if canvas is actually accessible
                    const canvasCheck = document.getElementById('outputCanvas');
                    if (!canvasCheck) {
                        throw new Error(`initVm failed: Canvas #outputCanvas not found in DOM. Error code: ${error} (${errorHex}). This indicates WebGL context creation failed because the canvas element is missing.`);
                    }
                    
                    // Check canvas dimensions
                    const canvasEl = canvasCheck as HTMLCanvasElement;
                    if (canvasEl.width === 0 || canvasEl.height === 0) {
                        throw new Error(`initVm failed: Canvas #outputCanvas has invalid dimensions (${canvasEl.width}x${canvasEl.height}). Error code: ${error} (${errorHex}). Canvas must have non-zero dimensions for WebGL context creation.`);
                    }
                    
                    // Check if canvas is in the document
                    if (!document.body.contains(canvasCheck)) {
                        throw new Error(`initVm failed: Canvas #outputCanvas is not in the document body. Error code: ${error} (${errorHex}). Canvas must be attached to the DOM for WebGL context creation.`);
                    }
                    
                    throw new Error(`initVm failed: WebGL context creation failed with error code ${error} (${errorHex}). Canvas exists with dimensions ${canvasEl.width}x${canvasEl.height}. This might indicate WebGL 2.0 is not available or the context attributes are incompatible.`);
                }
                
                // Check for Emscripten-specific error patterns
                const errorStr = String(error);
                if (errorStr.includes('assert') || errorStr.includes('Assertion')) {
                    throw new Error(`initVm assertion failed. This usually means WebGL context creation failed. Error: ${errorStr}`);
                }
                
                // Re-throw with more context
                if (error instanceof Error) {
                    throw new Error(`initVm failed: ${error.message}. The canvas #outputCanvas exists and has dimensions ${canvas.width}x${canvas.height}.`);
                } else {
                    throw new Error(`initVm failed with unexpected error type: ${typeof error}, value: ${error}`);
                }
            }
        } else if (typeof PlayModule._initVm === 'function') {
            // Call via EXPORTED_FUNCTIONS
            try {
                initResult = PlayModule._initVm();
                if (typeof initResult === 'number' && (initResult < 0 || initResult > 1000000)) {
                    throw new Error(`_initVm returned error code: ${initResult}`);
                }
            } catch (error: any) {
                throw new Error(`_initVm failed: ${error}`);
            }
        } else if (typeof PlayModule.ccall === 'function') {
            // Fallback to ccall
            try {
                initResult = PlayModule.ccall('initVm', null, [], []);
                if (typeof initResult === 'number' && (initResult < 0 || initResult > 1000000)) {
                    throw new Error(`initVm (via ccall) returned error code: ${initResult}`);
                }
            } catch (error: any) {
                throw new Error(`initVm (via ccall) failed: ${error}`);
            }
        } else {
            throw new Error('initVm function not found. Available methods: ' + Object.keys(PlayModule).filter(k => k.includes('init') || k === '_initVm').join(', '));
        }
        console.log('initVm completed successfully');
        
    } catch (error) {
        console.error('Error initializing PlayModule:', error);
        console.error('Error type:', typeof error);
        console.error('Error value:', error);
        
        // Log additional debugging info
        if (error instanceof Error) {
            console.error('Error message:', error.message);
            console.error('Error stack:', error.stack);
        } else if (typeof error === 'number') {
            // Emscripten error code
            console.error('Emscripten error code:', error);
            console.error('This might be a memory allocation error or initialization failure');
        } else {
            console.error('Error stringified:', JSON.stringify(error));
        }
        
        // Log PlayModule state if available
        if (PlayModule) {
            console.error('PlayModule state:', {
                hasHEAP8: !!PlayModule.HEAP8,
                hasHEAP32: !!PlayModule.HEAP32,
                hasFS: !!PlayModule.FS,
                hasccall: !!PlayModule.ccall
            });
        }
        
        throw error;
    }
};
