import './App.css';
import { ChangeEvent } from 'react';
import { PlayModule } from './PlayModule';
import { useAppDispatch, useAppSelector, loadElf } from "./Actions";

function App() {
  const state = useAppSelector((state) => state.play);
  const dispatch = useAppDispatch();
  const handleChange = function(event : ChangeEvent<HTMLInputElement>) {
    if(event.target && event.target.files && event.target.files.length !== 0) {
      var url = URL.createObjectURL(event.target.files[0]);
      dispatch(loadElf(url));
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
      </div>
    );
  }
}

export default App;
