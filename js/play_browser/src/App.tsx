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
  
  // Check for ROM URL parameter and load it automatically
  // Wait for PlayModule to be ready before loading ROM
  useEffect(() => {
    const urlParams = new URLSearchParams(window.location.search);
    const romUrl = urlParams.get('rom');
    const stateUrl = urlParams.get('state');
    
    if (!romUrl) {
      return; // No ROM URL, nothing to do
    }
    
    console.log('ROM URL found in query parameter:', romUrl);
    if (stateUrl) {
      console.log('State URL found in query parameter:', stateUrl);
    }
    
    // Function to load the state file
    const loadState = async (stateUrl: string) => {
      if (!PlayModule) {
        console.error('PlayModule not initialized for state loading');
        return;
      }
      
      try {
        console.log('Fetching state file from:', stateUrl);
        const response = await fetch(stateUrl);
        if (!response.ok) {
          throw new Error('Failed to fetch state: ' + response.status + ' ' + response.statusText);
        }
        
        const blob = await response.blob();
        console.log('State blob created, size:', blob.size);
        
        const arrayBuffer = await blob.arrayBuffer();
        const data = new Uint8Array(arrayBuffer);
        const statePath = "/work/savestate.zip";
        
        // Write the state file to Emscripten FS
        const stream = PlayModule.FS.open(statePath, "w+");
        PlayModule.FS.write(stream, data, 0, data.length, 0);
        PlayModule.FS.close(stream);
        
        // Load the state
        const success = PlayModule.loadState(statePath);
        if (success) {
          console.log('State loaded successfully from URL');
        } else {
          console.error('Failed to load state from URL');
        }
      } catch (error) {
        console.error('Error loading state file:', error);
      }
    };
    
    // Function to load the ROM
    const loadROM = () => {
      if (!PlayModule) {
        console.log('PlayModule not ready yet, waiting...');
        return false;
      }
      
      console.log('PlayModule ready, fetching ROM...');
      
      // Fetch ROM file from the ROM server
      fetch(romUrl)
        .then(response => {
          if (!response.ok) {
            throw new Error('Failed to fetch ROM: ' + response.status + ' ' + response.statusText);
          }
          console.log('ROM fetch started, size:', response.headers.get('content-length') || 'unknown');
          return response.blob();
        })
        .then(blob => {
          console.log('ROM blob created, size:', blob.size);
          
          // Extract filename from URL
          const urlParts = romUrl.split('/');
          const fileName = urlParts[urlParts.length - 1] || 'rom.iso';
          
          // Create File object from Blob
          const file = new File([blob], fileName, { type: blob.type || 'application/octet-stream' });
          console.log('ROM file created, size:', file.size);
          
          // Boot the ROM file
          dispatch(bootFile(file));
          
          // Notify Swift that ROM is loaded
          try {
            const win = window as any;
            if (win.webkit && win.webkit.messageHandlers && win.webkit.messageHandlers.romLoaded) {
              win.webkit.messageHandlers.romLoaded.postMessage({ loaded: true });
              console.log('Notified Swift via message handler');
            } else {
              // Fallback: try to call a global function if message handler not available
              if (win.romLoadedCallback) {
                win.romLoadedCallback();
                console.log('Notified Swift via callback');
              } else {
                console.log('No notification method available, ROM server will continue running');
              }
            }
          } catch (error) {
            console.error('Error notifying Swift:', error);
          }
          
          console.log('ROM loaded successfully');
          
          // Load state file if provided, wait a bit for ROM to initialize
          if (stateUrl) {
            setTimeout(() => {
              loadState(stateUrl);
            }, 2000); // Wait 2 seconds for ROM to boot
          }
        })
        .catch(error => {
          console.error('Error loading ROM file:', error);
        });
      
      return true; // Successfully started loading
    };
    
    // Try to load immediately if PlayModule is ready
    if (loadROM()) {
      return; // ROM loading started
    }
    
    // If PlayModule is not ready, poll for it
    console.log('Waiting for PlayModule to be ready...');
    const checkInterval = setInterval(() => {
      if (loadROM()) {
        clearInterval(checkInterval);
      }
    }, 100); // Check every 100ms
    
    // Cleanup: stop checking after 30 seconds (timeout)
    const timeout = setTimeout(() => {
      clearInterval(checkInterval);
      console.error('Timeout waiting for PlayModule to load ROM');
    }, 30000);
    
    return () => {
      clearInterval(checkInterval);
      clearTimeout(timeout);
    };
  }, [dispatch]);
  
  const handleChange = function(event : ChangeEvent<HTMLInputElement>) {
    if(event.target && event.target.files && event.target.files.length !== 0) {
      let file = event.target.files[0]; 
      dispatch(bootFile(file));
    }
  }
  
  const handleSaveState = async function() {
    if(PlayModule === null) {
      console.error("PlayModule not initialized");
      return;
    }
    try {
      const statePath = "/work/savestate.zip";
      const success = PlayModule.saveState(statePath);
      if(success) {
        // Read the saved state file from Emscripten FS
        const data = PlayModule.FS.readFile(statePath);
        const blob = new Blob([data], { type: "application/zip" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "savestate.zip";
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        console.log("Save state downloaded");
      } else {
        console.error("Failed to save state");
      }
    } catch(error) {
      console.error("Error saving state:", error);
    }
  }
  
  const handleLoadState = async function(event : ChangeEvent<HTMLInputElement>) {
    if(PlayModule === null) {
      console.error("PlayModule not initialized");
      return;
    }
    if(event.target && event.target.files && event.target.files.length !== 0) {
      try {
        const file = event.target.files[0];
        const arrayBuffer = await file.arrayBuffer();
        const data = new Uint8Array(arrayBuffer);
        const statePath = "/work/savestate.zip";
        
        // Write the uploaded file to Emscripten FS
        const stream = PlayModule.FS.open(statePath, "w+");
        PlayModule.FS.write(stream, data, 0, data.length, 0);
        PlayModule.FS.close(stream);
        
        // Load the state
        const success = PlayModule.loadState(statePath);
        if(success) {
          console.log("State loaded successfully");
        } else {
          console.error("Failed to load state");
        }
      } catch(error) {
        console.error("Error loading state:", error);
      }
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
        {/* disable file input b/c load from URL */}
        {/* <input type="file" onChange={handleChange} accept=".iso,.cso,.chd,.isz,.bin,.elf"/> */}
        <div>
          <button onClick={handleSaveState}>Download Save State</button>
          {/* <input type="file" id="loadStateInput" onChange={handleLoadState} accept=".bin,.zip" style={{display: "none"}}/> */}
          {/* <button onClick={() => document.getElementById("loadStateInput")?.click()}>Upload Load State</button> */}
        </div>
        <Stats />
      </div>
    );
  }
}

export default App;
