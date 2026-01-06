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
        const initVmResult = PlayModule.ccall("initVm", "", [], []);
        console.log('initVm completed, result:', initVmResult);
        
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
