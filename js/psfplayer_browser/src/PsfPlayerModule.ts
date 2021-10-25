import PsfPlayer from "./PsfPlayer";

export let PsfPlayerModule : any = null;

let module_overrides = {
    locateFile: function(path : string) {
        const baseURL = window.location.origin + window.location.pathname.substring(0, window.location.pathname.lastIndexOf( "/" ));
        return baseURL + '/' + path;
    },
    mainScriptUrlOrBlob: "",
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

function convertStringMapToDictionary(strMap : any) {
    let keysList = strMap.keys();
    let keys : string[] = [];
    for(let i = 0; i < keysList.size(); i++) {
        let str = keysList.get(i);
        keys.push(str);
    }
    let strDict : Map<string, string> = new Map();
    keys.forEach(key => strDict.set(key, strMap.get(key)));
    strMap.delete();
    return strDict;
}

export let initPsfPlayerModule = async function() {
    module_overrides.mainScriptUrlOrBlob = module_overrides.locateFile('PsfPlayer.js');
    PsfPlayerModule = await PsfPlayer(module_overrides);
    PsfPlayerModule.FS.mkdir("/work");
    PsfPlayerModule.ccall("initVm", "", [], []);
};

export let resumePsf = async function () {
    PsfPlayerModule.resumePsf();
}

export let pausePsf = async function () {
    PsfPlayerModule.pausePsf();
}

export let getPsfArchiveFileList = function(archivePath : string) {
    let fileList = PsfPlayerModule.getPsfArchiveFileList(archivePath);
    return convertStringVectorToArray(fileList);
}

export let loadPsfFromArchive = async function(archivePath : string, psfPath : string) {
    PsfPlayerModule.loadPsf(archivePath, psfPath);
}

export let getCurrentPsfTags = function() {
    let tags = convertStringMapToDictionary(PsfPlayerModule.getCurrentPsfTags());
    return tags;
}
