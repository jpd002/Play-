import './App.css';
import { ChangeEvent } from 'react';
import { PlayModule } from './PlayModule';
import { useAppDispatch, useAppSelector, bootFile } from "./Actions";

function App() {
  const state = useAppSelector((state) => state.play);
  const dispatch = useAppDispatch();
  const handleChange = function(event : ChangeEvent<HTMLInputElement>) {
    if(event.target && event.target.files && event.target.files.length !== 0) {
      let file = event.target.files[0]; 
      dispatch(bootFile(file));
    }
  }
  console.log(state.value);
  if(PlayModule === null) {
    return (
      <div>Loading...</div>
    );
  } else {
    return (
      <div>
        <input type="file" onChange={handleChange}/>
        <span className="version">
          {`Version: ${process.env.REACT_APP_VERSION}`}
        </span>
      </div>
    );
  }
}

export default App;
