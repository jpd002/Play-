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
        PlayModule = await Play(module_overrides);
        
        // Verify WASM memory is initialized
        if (!PlayModule || !PlayModule.HEAP8) {
            throw new Error('WASM module memory not properly initialized');
        }
        
        PlayModule.FS.mkdir("/work");
        PlayModule.discImageDevice = new DiscImageDevice(PlayModule);
        PlayModule.ccall("initVm", "", [], []);
    } catch (error) {
        console.error('Error initializing PlayModule:', error);
        // Log additional debugging info
        if (error instanceof Error) {
            console.error('Error message:', error.message);
            console.error('Error stack:', error.stack);
        }
        throw error;
    }
};
