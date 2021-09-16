import PsfPlayer from "./PsfPlayer";

export let PsfPlayerModule : any = null;

let module_overrides = {
    locateFile: function(path : string) {
        const baseURL = window.location.origin + window.location.pathname.substring(0, window.location.pathname.lastIndexOf( "/" ));
        return baseURL + '/' + path;
    }
};

function convertStringVectorToArray(strVector : any) {
    let strArray : string[] = [];
    for(let i = 0; i < strVector.size(); i++) {
        let str = strVector.get(i);
        strArray.push(str);
    }
    strVector.delete();
    return strArray;
}

export let initPsfPlayerModule = async function() {
    PsfPlayerModule = await PsfPlayer(module_overrides);
    PsfPlayerModule.FS.mkdir("/work");
    await PsfPlayerModule.ccall("initVm", "", [], [], { async: true });
};

export let getPsfArchiveFileList = function(archivePath : string) {
    let fileList = PsfPlayerModule.getPsfArchiveFileList(archivePath);
    return convertStringVectorToArray(fileList);
}

export let loadPsfFromArchive = async function(archivePath : string, psfPath : string) {
    await PsfPlayerModule.ccall("loadPsf", "", ['string', 'string'], [archivePath, psfPath], { async: true });
}

export let tickPsf = async function() {
    await PsfPlayerModule.ccall("step", "", [], [], { async: true });
}
