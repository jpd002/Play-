import './App.css';
import { ChangeEvent, useState, useEffect, useCallback } from 'react';
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

// Sanitize folder name (matching Swift FileHelper.sanitizeFolderName)
function sanitizeFolderName(name: string): string {
  const invalid = /[/\\?%*|"<>:]/g;
  const cleaned = name.replace(invalid, '_');
  return cleaned.trim();
}

function App() {
  const state = useAppSelector((state) => state.play);
  const dispatch = useAppDispatch();
  const [isLoading, setIsLoading] = useState(false);
  const [loadingMessage, setLoadingMessage] = useState("Loading...");
  const [romLoaded, setRomLoaded] = useState(false);
  const [stateLoaded, setStateLoaded] = useState(false);
  const [controlsExpanded, setControlsExpanded] = useState(true);
  const [selectedRomFileName, setSelectedRomFileName] = useState<string | null>(null);

  // Check if ROM and state are in URL parameters
  const [showFileInputs, setShowFileInputs] = useState(() => {
    const urlParams = new URLSearchParams(window.location.search);
    const hasRomParam = urlParams.get('rom') !== null;
    const hasStateParam = urlParams.get('state') !== null;
    return !hasRomParam && !hasStateParam;
  });

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

    // Initialize loading state
    setIsLoading(true);
    setLoadingMessage("Loading ROM...");
    setRomLoaded(false);
    setStateLoaded(!stateUrl); // If no state URL, mark as loaded immediately

    // Function to load the state file
    const loadState = async (stateUrl: string) => {
      if (!PlayModule) {
        console.error('PlayModule not initialized for state loading');
        setStateLoaded(true); // Mark as loaded even on error to hide loading
        return;
      }

      try {
        setLoadingMessage("Loading Save State...");
        console.log('Loading state file from:', stateUrl);

        // Check if URL is custom scheme (gameplaytoo://) or HTTP
        const isCustomScheme = stateUrl.startsWith('gameplaytoo://');

        let data: Uint8Array;

        if (isCustomScheme) {
          // Use message handler for custom scheme URLs
          const win = window as any;
          if (win.webkit && win.webkit.messageHandlers && win.webkit.messageHandlers.requestState) {
            // Request state data via message handler
            win.webkit.messageHandlers.requestState.postMessage(stateUrl);

            // Wait for state data via custom event
            data = await new Promise<Uint8Array>((resolve, reject) => {
              const handleStateData = (event: CustomEvent) => {
                resolve(event.detail);
                window.removeEventListener('stateDataReady', handleStateData as EventListener);
                window.removeEventListener('stateDataError', handleStateError as EventListener);
              };

              const handleStateError = (event: CustomEvent) => {
                reject(new Error(event.detail));
                window.removeEventListener('stateDataReady', handleStateData as EventListener);
                window.removeEventListener('stateDataError', handleStateError as EventListener);
              };

              window.addEventListener('stateDataReady', handleStateData as EventListener);
              window.addEventListener('stateDataError', handleStateError as EventListener);
            });
          } else {
            throw new Error('Message handler not available');
          }
        } else {
          // Use fetch for HTTP URLs (backward compatibility)
          const response = await fetch(stateUrl);
          if (!response.ok) {
            throw new Error('Failed to fetch state: ' + response.status + ' ' + response.statusText);
          }

          const blob = await response.blob();
          console.log('State blob created, size:', blob.size);

          const arrayBuffer = await blob.arrayBuffer();
          data = new Uint8Array(arrayBuffer);
        }

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
        setStateLoaded(true);
      } catch (error) {
        console.error('Error loading state file:', error);
        setStateLoaded(true); // Mark as loaded even on error to hide loading
      }
    };

    // Function to load the ROM
    const loadROM = () => {
      if (!PlayModule) {
        console.log('PlayModule not ready yet, waiting...');
        return false;
      }

      console.log('PlayModule ready, requesting ROM...');

      // Check if URL is custom scheme (gameplaytoo://) or HTTP
      const isCustomScheme = romUrl.startsWith('gameplaytoo://');

      if (isCustomScheme) {
        // Use message handler for custom scheme URLs
        try {
          const win = window as any;
          if (win.webkit && win.webkit.messageHandlers && win.webkit.messageHandlers.requestROM) {
            // Request ROM data via message handler
            win.webkit.messageHandlers.requestROM.postMessage({});

            // Set up event listener for ROM data
            const handleROMData = (event: CustomEvent) => {
              const file = event.detail;
              console.log('ROM file received, size:', file.size);

              // Extract filename
              const fileName = file.name || 'rom.iso';
              setSelectedRomFileName(fileName);

              // Boot the ROM file
              dispatch(bootFile(file));

              // Notify Swift that ROM is loaded
              try {
                if (win.webkit && win.webkit.messageHandlers && win.webkit.messageHandlers.romLoaded) {
                  win.webkit.messageHandlers.romLoaded.postMessage({ loaded: true });
                  console.log('Notified Swift via message handler');
                }
              } catch (error) {
                console.error('Error notifying Swift:', error);
              }

              console.log('ROM loaded successfully');
              setRomLoaded(true);

              // Load state file if provided, wait a bit for ROM to initialize
              if (stateUrl) {
                setTimeout(() => {
                  loadState(stateUrl);
                }, 2000); // Wait 2 seconds for ROM to boot
              }

              // Clean up event listener
              window.removeEventListener('romDataReady', handleROMData as EventListener);
            };

            const handleROMError = (event: CustomEvent) => {
              console.error('Error loading ROM file:', event.detail);
              setRomLoaded(true); // Mark as loaded even on error to hide loading
              setIsLoading(false);
              window.removeEventListener('romDataError', handleROMError as EventListener);
            };

            window.addEventListener('romDataReady', handleROMData as EventListener);
            window.addEventListener('romDataError', handleROMError as EventListener);
          } else {
            throw new Error('Message handler not available');
          }
        } catch (error) {
          console.error('Error requesting ROM via message handler:', error);
          setRomLoaded(true);
          setIsLoading(false);
        }
      } else {
        // Use fetch for HTTP URLs (backward compatibility)
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
            setSelectedRomFileName(fileName);

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
              }
            } catch (error) {
              console.error('Error notifying Swift:', error);
            }

            console.log('ROM loaded successfully');
            setRomLoaded(true);

            // Load state file if provided, wait a bit for ROM to initialize
            if (stateUrl) {
              setTimeout(() => {
                loadState(stateUrl);
              }, 2000); // Wait 2 seconds for ROM to boot
            }
          })
          .catch(error => {
            console.error('Error loading ROM file:', error);
            setRomLoaded(true); // Mark as loaded even on error to hide loading
            setIsLoading(false);
          });
      }

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

  const handleChange = function (event: ChangeEvent<HTMLInputElement>) {
    if (event.target && event.target.files && event.target.files.length !== 0) {
      let file = event.target.files[0];
      setSelectedRomFileName(file.name);
      dispatch(bootFile(file));
      setRomLoaded(true);
    }
  }

  const handleSaveState = useCallback(async function () {
    if (PlayModule === null) {
      console.error("PlayModule not initialized");
      return;
    }
    try {
      const statePath = "/work/savestate.zip";
      const success = PlayModule.saveState(statePath);
      if (success) {
        // Read the saved state file from Emscripten FS
        const data = PlayModule.FS.readFile(statePath);
        const blob = new Blob([data], { type: "application/zip" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;

        // Generate download filename from ROM name
        let downloadFileName = "savestate.bin";
        if (selectedRomFileName) {
          // Remove file extension from ROM filename
          const romNameWithoutExt = selectedRomFileName.replace(/\.[^/.]+$/, "");
          const sanitizedRomName = sanitizeFolderName(romNameWithoutExt);
          downloadFileName = `${sanitizedRomName}__state.bin`;
        }

        a.download = downloadFileName;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        console.log("Save state downloaded");
      } else {
        console.error("Failed to save state");
      }
    } catch (error) {
      console.error("Error saving state:", error);
    }
  }, [selectedRomFileName]);

  // Expose handleSaveState to global window object so Swift can call it
  useEffect(() => {
    (window as any).handleSaveState = handleSaveState;
    return () => {
      delete (window as any).handleSaveState;
    };
  }, [handleSaveState]);

  // Hide loading modal when both ROM and state (if any) are loaded
  useEffect(() => {
    if (romLoaded && stateLoaded) {
      // Small delay to ensure everything is ready
      setTimeout(() => {
        setIsLoading(false);
      }, 500);
    }
  }, [romLoaded, stateLoaded]);

  const handleLoadState = async function (event: ChangeEvent<HTMLInputElement>) {
    if (PlayModule === null) {
      console.error("PlayModule not initialized");
      return;
    }
    if (event.target && event.target.files && event.target.files.length !== 0) {
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
        if (success) {
          console.log("State loaded successfully");
        } else {
          console.error("Failed to load state");
        }
      } catch (error) {
        console.error("Error loading state:", error);
      }
    }
  }

  console.log(state.value);
  if (PlayModule === null) {
    return (
      <div style={{
        position: 'fixed',
        top: 0,
        left: 0,
        width: '100%',
        height: '100%',
        backgroundColor: 'rgba(0, 0, 0, 0.95)',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        zIndex: 9999,
        color: 'white',
        fontFamily: 'system-ui, -apple-system, sans-serif'
      }}>
        <div style={{
          fontSize: '32px',
          marginBottom: '20px',
          animation: 'spin 1s linear infinite'
        }}>
          ⏳
        </div>
        <div style={{
          fontSize: '24px',
          fontWeight: '500'
        }}>
          Loading Emulator...
        </div>
        <style>{`
          @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
          }
        `}</style>
      </div>
    );
  } else {
    return (
      <div>
        {/* Loading Modal */}
        {isLoading && (
          <div style={{
            position: 'fixed',
            top: 0,
            left: 0,
            width: '100%',
            height: '100%',
            backgroundColor: 'rgba(0, 0, 0, 0.95)',
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 9999,
            color: 'white',
            fontFamily: 'system-ui, -apple-system, sans-serif'
          }}>
            <div style={{
              fontSize: '32px',
              marginBottom: '20px',
              animation: 'spin 1s linear infinite'
            }}>
              ⏳
            </div>
            <div style={{
              fontSize: '24px',
              fontWeight: '500'
            }}>
              {loadingMessage}
            </div>
            <style>{`
              @keyframes spin {
                from { transform: rotate(0deg); }
                to { transform: rotate(360deg); }
              }
            `}</style>
          </div>
        )}

        {/* Show file inputs only if no ROM/state in URL parameters */}
        {showFileInputs && (
          <>
            {/* Toggle Button - Always visible when controls are available */}
            <button
              onClick={() => setControlsExpanded(!controlsExpanded)}
              style={{
                position: 'fixed',
                bottom: '20px',
                right: '20px',
                width: '56px',
                height: '56px',
                borderRadius: '28px',
                backgroundColor: controlsExpanded ? '#000000' : 'rgba(0, 0, 0, 0.6)',
                color: 'white',
                border: '3px solid rgba(255, 255, 255, 0.5)',
                cursor: 'pointer',
                zIndex: 1001,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontSize: '24px',
                boxShadow: '0 4px 12px rgba(0, 0, 0, 0.3)',
                transition: 'all 0.2s ease',
                WebkitTapHighlightColor: 'transparent',
                touchAction: 'manipulation'
              }}
              onTouchStart={(e) => e.currentTarget.style.transform = 'scale(0.95)'}
              onTouchEnd={(e) => e.currentTarget.style.transform = 'scale(1)'}
              aria-label={controlsExpanded ? 'Hide controls' : 'Show controls'}
            >
              {controlsExpanded ? '✕' : '☰'}
            </button>

            {/* Collapsible Controls Panel */}
            <div
              style={{
                position: 'fixed',
                top: 0,
                left: 0,
                right: 0,
                backgroundColor: 'rgba(0, 0, 0, 0.2)',
                backdropFilter: 'blur(10px)',
                WebkitBackdropFilter: 'blur(10px)',
                zIndex: 1000,
                padding: '20px',
                paddingTop: 'max(20px, env(safe-area-inset-top))',
                display: 'flex',
                flexDirection: 'column',
                gap: '15px',
                alignItems: 'center',
                transform: controlsExpanded ? 'translateY(0)' : 'translateY(-100%)',
                opacity: controlsExpanded ? 1 : 0,
                pointerEvents: controlsExpanded ? 'auto' : 'none',
                transition: 'transform 0.3s ease, opacity 0.3s ease',
                boxShadow: controlsExpanded ? '0 4px 12px rgba(0, 0, 0, 0.3)' : 'none'
              }}
            >
              <div style={{
                display: 'flex',
                gap: '12px',
                flexWrap: 'wrap',
                justifyContent: 'center',
                width: '100%',
                maxWidth: '400px'
              }}>
                <input
                  type="file"
                  id="romFileInput"
                  onChange={handleChange}
                  accept=".iso,.cso,.chd,.isz,.bin,.elf"
                  style={{ display: "none" }}
                />
                <button
                  onClick={() => !romLoaded && document.getElementById("romFileInput")?.click()}
                  // disabled={romLoaded}
                  style={{
                    padding: '12px',
                    backgroundColor: romLoaded ? 'rgba(0, 0, 0, 0.4)' : 'rgba(0, 0, 0, 0.6)',
                    color: 'white',
                    border: '3px solid rgba(255, 255, 255, 0.5)',
                    borderRadius: '50px',
                    cursor: romLoaded ? 'not-allowed' : 'pointer',
                    fontSize: '16px',
                    fontWeight: '500',
                    minHeight: '44px',
                    flex: '1',
                    minWidth: '140px',
                    WebkitTapHighlightColor: 'transparent',
                    touchAction: 'manipulation',
                    transition: 'opacity 0.2s ease',
                    // opacity: romLoaded ? 0.6 : 1,
                    opacity: 1,
                  }}
                  onTouchStart={(e) => !romLoaded && (e.currentTarget.style.opacity = '0.7')}
                  onTouchEnd={(e) => !romLoaded && (e.currentTarget.style.opacity = '1')}
                >
                  {selectedRomFileName || 'Select the ROM file from any location (e.g. On my iPhone/iPad → GamePlaytoo → roms)'}
                </button>
              </div>
              <div style={{
                display: 'flex',
                flexDirection: 'column',
                gap: '12px',
                justifyContent: 'center',
                width: '100%',
                maxWidth: '400px'
              }}>
                <button
                  onClick={handleSaveState}
                  style={{
                    padding: '12px',
                    backgroundColor: 'rgba(0, 0, 0, 0.6)',
                    color: 'white',
                    border: '3px solid rgba(255, 255, 255, 0.5)',
                    borderRadius: '50px',
                    cursor: 'pointer',
                    fontSize: '16px',
                    fontWeight: '500',
                    minHeight: '44px',
                    width: '100%',
                    WebkitTapHighlightColor: 'transparent',
                    touchAction: 'manipulation',
                    transition: 'opacity 0.2s ease',
                    display: romLoaded ? 'block' : 'none',
                  }}
                  onTouchStart={(e) => e.currentTarget.style.opacity = '0.7'}
                  onTouchEnd={(e) => e.currentTarget.style.opacity = '1'}
                >
                  Save state to a file (On my iPhone/iPad → GamePlaytoo → states → [ROM Name])
                </button>
                <input
                  type="file"
                  id="loadStateInput"
                  onChange={handleLoadState}
                  accept=".bin,.zip"
                  style={{ display: "none" }}
                />
                <button
                  onClick={() => document.getElementById("loadStateInput")?.click()}
                  style={{
                    padding: '12px',
                    backgroundColor: 'rgba(0, 0, 0, 0.6)',
                    color: 'white',
                    border: '3px solid rgba(255, 255, 255, 0.5)',
                    borderRadius: '50px',
                    cursor: 'pointer',
                    fontSize: '16px',
                    fontWeight: '500',
                    minHeight: '44px',
                    width: '100%',
                    WebkitTapHighlightColor: 'transparent',
                    touchAction: 'manipulation',
                    transition: 'opacity 0.2s ease',
                    display: romLoaded ? 'block' : 'none',
                  }}
                  onTouchStart={(e) => e.currentTarget.style.opacity = '0.7'}
                  onTouchEnd={(e) => e.currentTarget.style.opacity = '1'}
                >
                  Load state from a file (On my iPhone/iPad → GamePlaytoo → states → [ROM Name])
                </button>
              </div>
            </div>
          </>
        )
        }
        <Stats />
      </div >
    );
  }
}

export default App;
