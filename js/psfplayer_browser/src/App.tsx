import './App.css';
import { ChangeEvent } from 'react';
import { RootState, useAppDispatch, useAppSelector, init, loadArchive, loadPsf, play, stop } from "./Actions";
import { PsfPlayerModule } from './PsfPlayerModule';

export default function App() {
    const dispatch = useAppDispatch();
    const state = useAppSelector((state) => state.player);
    const handleChange = function(event : ChangeEvent<HTMLInputElement>) {
      if(event.target && event.target.files) {
        var url = URL.createObjectURL(event.target.files[0]);
        dispatch(loadArchive(url));
      }
      console.log("Uploading file...");
    }
    const handleClick = function(psfFilePath : string) {
      dispatch(loadPsf(psfFilePath));
      console.log(`Clicked ${psfFilePath}!`);
    }
    if(PsfPlayerModule === null) {
      return (
        <div className="App">
          <button onClick={() => dispatch(init())}>Init</button>
        </div>
      )
    } else {
      return (
        <div className="App">
          <div>
            <textarea value={state.value} />
            <br />
            <button onClick={() => dispatch(play())}>Start</button>
            <button onClick={() => dispatch(stop())}>Stop</button>
          </div>
          <div>
            <input type="file" onChange={handleChange}/>
          </div>
          <div>
            <ul>
              {
                state.archiveFileList.map(item => (
                  <li><a onClick={(e) => handleClick(item)} style={{cursor: 'pointer'}}>{item}</a></li>
                ))
              }
            </ul>
          </div>
        </div>
      );
    }
};
