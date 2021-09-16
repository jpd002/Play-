export class AudioState
{
    state : number;

    constructor() {
        this.state = 0;
    }

    startAudio2() {
        var audioContext = new AudioContext();
    
        const buffer = audioContext.createBuffer(2, 256, 44100);
        for(let channel = 0; channel < 2; channel++)
        {
            const bufferData = buffer.getChannelData(channel);
            for(let i = 0; i < bufferData.length; i++)
            {
                bufferData[i] = Math.random() * 2 - 1;
            }
        }
        
        const source = audioContext.createBufferSource();
        source.buffer = buffer;
        source.loop = true;
        
        source.connect(audioContext.destination);
        source.start(0);
    
        console.log(audioContext);
    }
    
    startAudio() {
        this.state = 1;
    }

    stopAudio() {
        this.state = 2;
    }
    
    getState() {
        return this.state.toString();
    }
}
