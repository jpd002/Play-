import { configureStore, createAsyncThunk, createReducer } from "@reduxjs/toolkit";
import { TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import { PsfPlayerModule, initPsfPlayerModule, resumePsf, pausePsf, getPsfArchiveFileList, loadPsfFromArchive, getCurrentPsfTags } from "./PsfPlayerModule";

const invalidPlayingIndex = -1;

let archiveFilePath = "archive.zip";

const loadPsfFromPath = async function(psfFilePath : string) {
    await loadPsfFromArchive(archiveFilePath, psfFilePath);
    let tags = getCurrentPsfTags();
    return tags;
}

const playPsf = async function() {
    resumePsf();
}

const stopPsf = async function() {
    pausePsf();
}

export type AudioState = {
    value: string,
    psfLoaded: boolean
    playing: boolean
    playingIndex: number
    archiveFileList : string[]
    archiveFileListVersion : number;
    currentPsfTags : any | undefined
};

type LoadPsfResult = {
    archiveFileIndex : number
    tags : any,
};

let initialState : AudioState = {
    value: "unknown",
    psfLoaded: false,
    playing: false,
    playingIndex: invalidPlayingIndex,
    archiveFileList: [],
    archiveFileListVersion: 0,
    currentPsfTags: undefined
};

export const init = createAsyncThunk<void>('init', 
    async () => {
        await initPsfPlayerModule();
    }
);

export const loadArchive = createAsyncThunk<string[] | undefined, File>('loadArchive',
    async (file : File, thunkAPI) => {
        let url = URL.createObjectURL(file);
        console.log(`loading ${url}...`);
        await stopPsf();
        let blob = await fetch(url).then(response => {
            if(!response.ok) {
                return null;
            } else {
                return response.blob();
            }
        });
        if(blob === null) {
            thunkAPI.rejectWithValue(null);
            return;
        }
        let data = new Uint8Array(await blob.arrayBuffer());
        let stream = PsfPlayerModule.FS.open(archiveFilePath, "w+");
        PsfPlayerModule.FS.write(stream, data, 0, data.length, 0);
        PsfPlayerModule.FS.close(stream);
        URL.revokeObjectURL(url);
        let fileList = getPsfArchiveFileList(archiveFilePath);
        fileList = fileList.filter(path => 
            path.endsWith(".psf") ||
            path.endsWith(".psf2") ||
            path.endsWith(".minipsf") ||
            path.endsWith(".minipsf2"));
        return fileList;
    }
);

export const loadPsf = createAsyncThunk<LoadPsfResult, number>('loadPsf',
    async (archiveFileIndex : number, thunkAPI) => {
        let state = thunkAPI.getState() as RootState;
        let psfFilePath = state.player.archiveFileList[archiveFileIndex];
        let tags = await loadPsfFromPath(psfFilePath);
        let result : LoadPsfResult = {
            archiveFileIndex: archiveFileIndex,
            tags: Object.fromEntries(tags)
        };
        return result;
    }
);

export const play = createAsyncThunk<void, void>('play',
    async () => {
        await playPsf();
    }
);

export const pause = createAsyncThunk<void, void>('pause',
    async() => {
        await stopPsf();
    }
);

const reducer = createReducer(initialState, (builder) => (
    builder
        .addCase(init.fulfilled, (state, action) => {
            console.log("init");
            state.value = "initialized";
            return state;
        })
        .addCase(loadArchive.fulfilled, (state, action) => {
            state.value = "loaded";
            state.playingIndex = invalidPlayingIndex;
            state.currentPsfTags = undefined;
            if(action.payload) {
                state.archiveFileList = action.payload;
            } else {
                state.archiveFileList = [];
            }
            state.archiveFileListVersion = state.archiveFileListVersion + 1;
            return state;
        })
        .addCase(loadArchive.rejected, (state, action) => {
            state.playingIndex = invalidPlayingIndex;
            state.currentPsfTags = undefined;
            state.archiveFileList = [];
            state.archiveFileListVersion = state.archiveFileListVersion + 1;
            state.value = `loading failed: ${action.error.message}`;
            return state;
        })
        .addCase(loadPsf.fulfilled, (state, action) => {
            state.value = "psf loaded";
            state.currentPsfTags = action.payload.tags;
            state.playingIndex = action.payload.archiveFileIndex;
            state.psfLoaded = true;
            state.playing = true;
            return state;
        })
        .addCase(loadPsf.rejected, (state, action) => {
            state.value = `loading failed: ${action.error.message}`;
            return state;
        })
        .addCase(play.fulfilled, (state) => {
            state.value = "playing";
            state.playing = true;
            return state;
        })
        .addCase(pause.fulfilled, (state) => {
            state.value = "paused";
            state.playing = false;
            return state;
        })
));

export const store = configureStore({
    reducer: { player: reducer }
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;

export const useAppDispatch = () => useDispatch<AppDispatch>();
export const useAppSelector: TypedUseSelectorHook<RootState> = useSelector;
