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
    let factory = require("../../build_cmake/build_wasm/tools/PsfPlayer/Source/js_ui/PsfPlayer.js");
    let module = await factory();
    module.FS.mkdir("/work");
    module.FS.mount(module.NODEFS, { root: "."}, "/work");

    let psfArchivePath = "work/Final Fantasy VII (EMU).zophar.zip";
    let psfArchiveFileList = convertStringVectorToArray(module.getPsfArchiveFileList(psfArchivePath));

    await module.ccall("initVm", "", [], [], { async: true });
    await module.ccall("loadPsf", "", ['string', 'string'], [psfArchivePath, psfArchiveFileList[0]], { async: true });
    while(true) {
        await module.ccall("step", "", [], [], { async: true });
    }
    console.log("Finished execution.");
}();
