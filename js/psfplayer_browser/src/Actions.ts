import { configureStore, createAction, createAsyncThunk, createReducer } from "@reduxjs/toolkit";
import { waitFor } from "@testing-library/react";
import { url } from "inspector";
import { TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import { PsfPlayerModule, initPsfPlayerModule, getPsfArchiveFileList, loadPsfFromArchive, tickPsf } from "./PsfPlayerModule";
import { Mutex } from 'async-mutex';
import { release } from "os";

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

export type AudioState = {
    value: string,
    archiveFileList : string[]
};

let initialState : AudioState = {
    value: "unknown",
    archiveFileList: [],
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
        return fileList;
    }
);
export const loadPsf = createAsyncThunk<void, string>('loadPsf',
    async (psfFilePath : string, thunkAPI) => {
        let releaseLock = await tickMutex.acquire();
        clearTimeout(updateTimer);
        await loadPsfFromArchive(archiveFilePath, psfFilePath);
        updateTimer = setTimeout(updateFct, updateDelay);
        releaseLock();
    }
);
export const play = createAsyncThunk<void, void>('play',
    async () => {
        let releaseLock = await tickMutex.acquire();
        updateTimer = setTimeout(updateFct, updateDelay);
        releaseLock();
    }
);
export const stop = createAsyncThunk<void, void>('stop',
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
            return state;
        })
        .addCase(loadPsf.rejected, (state, action) => {
            state.value = `loading failed: ${action.error.message}`;
            return state;
        })
        .addCase(play.fulfilled, (state) => {
            console.log("playing");
            state.value = "playing";
            return state;
        })
        .addCase(stop.fulfilled, (state) => {
            console.log("stopping");
            state.value = "stopped";
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
