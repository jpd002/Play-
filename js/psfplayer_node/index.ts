function convertStringVectorToArray(strVector : any) {
    let strArray : string[] = [];
    for(let i = 0; i < strVector.size(); i++) {
        let str = strVector.get(i);
        strArray.push(str);
    }
    strVector.delete();
    return strArray;
}

var main = async function() {
    let factory = require("../../../build_cmake/build_wasm/tools/PsfPlayer/Source/js_ui/PsfPlayer.js");
    let module = await factory();
    module.FS.mkdir("/work");
    module.FS.mount(module.NODEFS, { root: "."}, "/work");

    let psfArchivePath = "work/psf/Valkyrie Profile (EMU).zophar.zip";
    let psfArchiveFileList = convertStringVectorToArray(module.getPsfArchiveFileList(psfArchivePath));

    module.ccall("initVm");
    module.loadPsf(psfArchivePath, psfArchiveFileList[0]);

    console.log("Finished initialization.");
}();
