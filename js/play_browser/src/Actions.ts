import { configureStore, createAsyncThunk, createReducer } from "@reduxjs/toolkit";
import { TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import { PlayModule, initPlayModule } from './PlayModule';

export type AudioState = {
    value: string,
};

let initialState : AudioState = {
    value: "unknown",
};

export const init = createAsyncThunk<void>('init', 
    async () => {
        await initPlayModule();
    }
);

export const bootFile = createAsyncThunk<void, File>('bootFile',
    async (file : File, thunkAPI) => {
        const fileName = file.name;
        const fileDotPos = fileName.lastIndexOf('.');
        if(fileDotPos === -1) {
            console.log("File name must have an extension.");
            thunkAPI.rejectWithValue(null);
            return;
        }
        const fileExtension = fileName.substring(fileDotPos);
        if(fileExtension === ".elf") {
            let url = URL.createObjectURL(file);
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
            let stream = PlayModule.FS.open(fileName, "w+");
            PlayModule.FS.write(stream, data, 0, data.length, 0);
            PlayModule.FS.close(stream);
            URL.revokeObjectURL(url);
            PlayModule.bootElf(fileName);
        } else {
            PlayModule.discImageDevice.setFile(file);
            PlayModule.bootDiscImage(fileName);
        }
    }
);

const reducer = createReducer(initialState, (builder) => (
    builder
        .addCase(init.fulfilled, (state, action) => {
            console.log("init");
            state.value = "initialized";
            return state;
        })
        .addCase(bootFile.fulfilled, (state, action) => {
            state.value = "loaded";
            return state;
        })
        .addCase(bootFile.rejected, (state, action) => {
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
