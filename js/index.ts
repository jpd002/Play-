var main = async function() {
    var factory = require("../../build_cmake/build_wasm/tools/PsfPlayer/Source/js_ui/PsfPlayer.js");
    var module = await factory();
    module.FS.mkdir("/work");
    module.FS.mount(module.NODEFS, { root: "."}, "/work");
    await module.ccall("initVm", "", [], [], { async: true });
    await module.ccall("loadPsf", "", [], [], { async: true });
    while(true) {
        await module.ccall("step", "", [], [], { async: true });
    }
    console.log("Finished execution.");
}();
