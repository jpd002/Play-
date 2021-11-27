import Play from "./Play";

export let PlayModule : any = null;

let module_overrides = {
    locateFile: function(path : string) {
        const baseURL = window.location.origin + window.location.pathname.substring(0, window.location.pathname.lastIndexOf( "/" ));
        return baseURL + '/' + path;
    },
    mainScriptUrlOrBlob: "",
};

export let initPlayModule = async function() {
    module_overrides.mainScriptUrlOrBlob = module_overrides.locateFile('Play.js');
    PlayModule = await Play(module_overrides);
    PlayModule.FS.mkdir("/work");
    PlayModule.ccall("initVm", "", [], []);
};
