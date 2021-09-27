import './App.css';
import { ChangeEvent } from 'react';
import { FixedSizeList, ListChildComponentProps } from 'react-window';
import { ListItem, ListItemButton, ListItemText } from '@mui/material';
import { useAppDispatch, useAppSelector, loadArchive, loadPsf, play, pause } from "./Actions";
import { PsfPlayerModule } from './PsfPlayerModule';

function RenderRow(props: ListChildComponentProps) {
  const { index, style } = props;

  const state = useAppSelector((state) => state.player);
  let archiveFile = state.archiveFileList[index];

  const dispatch = useAppDispatch();  
  const handleClick = function(archiveFileIndex : number) {
    dispatch(loadPsf(archiveFileIndex));
  }

  let itemStyle : React.CSSProperties = {};
  if(index === state.playingIndex) {
    itemStyle.fontWeight = "bold";
  }

  return (
    <ListItem style={style} key={index} component="div" disablePadding>
      <ListItemButton onClick={(e) => handleClick(index)}>
        <ListItemText primary={archiveFile} primaryTypographyProps={{style: itemStyle}} />
      </ListItemButton>
    </ListItem>
  );
}

export default function App() {
    const dispatch = useAppDispatch();
    const state = useAppSelector((state) => state.player);
    const handleChange = function(event : ChangeEvent<HTMLInputElement>) {
      if(event.target && event.target.files && event.target.files.length !== 0) {
        var url = URL.createObjectURL(event.target.files[0]);
        dispatch(loadArchive(url));
      }
    }
    if(PsfPlayerModule === null) {
      return (
        <div className="App">
          <span>Loading...</span>
        </div>
      )
    } else {
      let prevIndex = Math.max(state.playingIndex - 1, 0);
      let nextIndex = Math.min(state.playingIndex + 1, state.archiveFileList.length);
      return (
        <div className="App">
          <FixedSizeList
            className={"playlist"}
            height={480}
            width={360}
            itemSize={46}
            itemCount={state.archiveFileList.length}
            overscanCount={5}
            >
              {RenderRow}
          </FixedSizeList>
          <div>
            <br />
            <div>{state.currentPsfTags ? `${state.currentPsfTags.game} - ${state.currentPsfTags.title}` : 'PsfPlayer'}</div>
            <br />
            <button disabled={!state.psfLoaded} onClick={() => dispatch(loadPsf(prevIndex))}>&#x23EE;</button>
            {
              state.playing ?
                (<button disabled={!state.psfLoaded} onClick={() => dispatch(pause())}>&#x23F8;</button>)
                :
                (<button disabled={!state.psfLoaded} onClick={() => dispatch(play())}>&#x25B6;</button>)
            }
            <button disabled={!state.psfLoaded} onClick={() => dispatch(loadPsf(nextIndex))}>&#x23ED;</button>
          </div>
          <div>
            <input type="file" onChange={handleChange}/>
          </div>
          <span className="version">
            {`Version: ${process.env.REACT_APP_VERSION}`}
          </span>
        </div>
      );
    }
};
