import './App.css';
import { ChangeEvent, useState, useEffect } from 'react';
import { PlayModule } from './PlayModule';
import { useAppDispatch, useAppSelector, bootFile } from "./Actions";

function Stats() {
  const [stats, setStats] = useState({ fps: 0 });
  useEffect(() => {
    const timer = setInterval(() => {
      const frames = PlayModule.getFrames();
      PlayModule.clearStats();
      setStats({ fps: frames }); 
    }, 1000);
    return () => {
      clearInterval(timer);
    };
  }, [stats]);
  return (
    <span className="stats">{stats.fps} f/s</span>
  )
}

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
        <Stats />
        <span className="version">
          {`Version: ${process.env.REACT_APP_VERSION}`}
        </span>
      </div>
    );
  }
}

export default App;
