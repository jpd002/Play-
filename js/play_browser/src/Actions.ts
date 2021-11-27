import { configureStore, createAsyncThunk, createReducer } from "@reduxjs/toolkit";
import { TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import { PlayModule, initPlayModule } from './PlayModule';

let filePath = "archive.zip";

export type AudioState = {
    value: string,
    psfLoaded: boolean
    playing: boolean
    archiveFileList : string[]
    currentPsfTags : any | undefined
};

let initialState : AudioState = {
    value: "unknown",
    psfLoaded: false,
    playing: false,
    archiveFileList: [],
    currentPsfTags: undefined
};

export const init = createAsyncThunk<void>('init', 
    async () => {
        await initPlayModule();
    }
);

export const loadElf = createAsyncThunk<void, string>('loadElf',
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
        let stream = PlayModule.FS.open(filePath, "w+");
        PlayModule.FS.write(stream, data, 0, data.length, 0);
        PlayModule.FS.close(stream);
        PlayModule.loadElf(filePath);
    }
);

const reducer = createReducer(initialState, (builder) => (
    builder
        .addCase(init.fulfilled, (state, action) => {
            console.log("init");
            state.value = "initialized";
            return state;
        })
        .addCase(loadElf.fulfilled, (state, action) => {
            state.value = "loaded";
            state.currentPsfTags = undefined;
            return state;
        })
        .addCase(loadElf.rejected, (state, action) => {
            state.currentPsfTags = undefined;
            state.archiveFileList = [];
            state.value = `loading failed: ${action.error.message}`;
            return state;
        })
));

export const store = configureStore({
    reducer: { play: reducer }
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;

export const useAppDispatch = () => useDispatch<AppDispatch>();
export const useAppSelector: TypedUseSelectorHook<RootState> = useSelector;
