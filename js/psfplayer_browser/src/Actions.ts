import { configureStore, createAsyncThunk, createReducer } from "@reduxjs/toolkit";
import { TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import { PsfPlayerModule, initPsfPlayerModule, getPsfArchiveFileList, loadPsfFromArchive, getCurrentPsfTags, tickPsf } from "./PsfPlayerModule";
import { Mutex } from 'async-mutex';

let archiveFilePath = "archive.zip";
let tickMutex = new Mutex();
const updateDelay : number = 30;
let updateTimer : any;

let updateFct = async function() {
    let releaseLock = await tickMutex.acquire();
    await tickPsf();
    updateTimer = setTimeout(updateFct, updateDelay);
    releaseLock();
};

const loadPsfFromPath = async function(psfFilePath : string) {
    let releaseLock = await tickMutex.acquire();
    clearTimeout(updateTimer);
    await loadPsfFromArchive(archiveFilePath, psfFilePath);
    let tags = getCurrentPsfTags();
    updateTimer = setTimeout(updateFct, updateDelay);
    releaseLock();
    return tags;
}

export type AudioState = {
    value: string,
    psfLoaded: boolean
    playing: boolean
    playingIndex: number
    archiveFileList : string[]
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
    playingIndex: -1,
    archiveFileList: [],
    currentPsfTags: undefined
};

export const init = createAsyncThunk<void>('init', 
    async () => {
        await initPsfPlayerModule();
    }
);

export const loadArchive = createAsyncThunk<string[] | undefined, string>('loadArchive',
    async (url : string, thunkAPI) => {
        console.log(`loading ${url}...`);
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
        let releaseLock = await tickMutex.acquire();
        updateTimer = setTimeout(updateFct, updateDelay);
        releaseLock();
    }
);

export const pause = createAsyncThunk<void, void>('pause',
    async() => {
        let releaseLock = await tickMutex.acquire();
        clearTimeout(updateTimer);
        releaseLock();
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
            if(action.payload) {
                state.archiveFileList = action.payload;
            } else {
                state.archiveFileList = [];
            }
            return state;
        })
        .addCase(loadArchive.rejected, (state, action) => {
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
            console.log("playing");
            state.value = "playing";
            state.playing = true;
            return state;
        })
        .addCase(pause.fulfilled, (state) => {
            console.log("paused");
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
