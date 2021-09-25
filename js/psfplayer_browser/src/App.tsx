import './App.css';
import { ChangeEvent } from 'react';
import { FixedSizeList, ListChildComponentProps } from 'react-window';
import { ListItem, ListItemButton, ListItemText } from '@mui/material';
import { useAppDispatch, useAppSelector, loadArchive, loadPsf, play, stop } from "./Actions";
import { PsfPlayerModule } from './PsfPlayerModule';

function RenderRow(props: ListChildComponentProps) {
  const { index, style, data } = props;
  let archiveFileList = data as string[];
  let archiveFile = archiveFileList[index];

  const dispatch = useAppDispatch();  
  const handleClick = function(psfFilePath : string) {
    dispatch(loadPsf(psfFilePath));
  }

  return (
    <ListItem style={style} key={index} component="div" disablePadding>
      <ListItemButton onClick={(e) => handleClick(archiveFile)}>
        <ListItemText primary={archiveFile} />
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
      return (
        <div className="App">
          <FixedSizeList
            className={"playlist"}
            height={480}
            width={360}
            itemSize={46}
            itemCount={state.archiveFileList.length}
            overscanCount={5}
            itemData={state.archiveFileList}
            >
              {RenderRow}
          </FixedSizeList>
          <div>
            <br />
            <div>{state.currentPsfTags ? `${state.currentPsfTags.game} - ${state.currentPsfTags.title}` : 'PsfPlayer'}</div>
            <br />
            <button>&#x23EE;</button>
            {
              state.playing ?
                (<button disabled={!state.psfLoaded} onClick={() => dispatch(stop())}>&#x23F8;</button>)
                :
                (<button disabled={!state.psfLoaded} onClick={() => dispatch(play())}>&#x25B6;</button>)
            }
            <button>&#x23ED;</button>
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
