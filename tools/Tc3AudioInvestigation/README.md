# TC3 movie audio investigation (experimental)

The Japanese tst1vera opening reuses a movie bank before its audio producer has finished with the packet header. At movie frame 374 the selected bank contains packet 10 (14 audio blocks); at frame 377 it already contains packet 12 (17 blocks), while the audio producer still uses index 14. This sends earlier/future audio instead of switching to packet 11. The nominal audio rate is unchanged.

## Changes

- Fix Namco CDVD streaming sector indexing: the stream position already increments inside the loop.
- Preserve fractional PCM source position between DMA blocks (48 kHz input to 44.1 kHz output otherwise loses about 0.34%).
- Add ROM-free regression tests for both corrections.
- Include an opt-in diagnostic arcade definition and an MPS/PCM comparison script. The two core corrections alone do not fix the TC3 bank race.

## Experimental definition

The preview definition is NOT installed by default. It is specific to the local Japanese tst1vera executable and redirects read submission at 0x14c540 to a diagnostic stub at 0x000f0000. While the destination bank is still owned by the audio producer, it pumps only the original audio-transfer portion before submitting the read. The code cave was checked as zero in the local opening RAM dump; it is not a general allocation mechanism or safe for other revisions. Review the machine-code patch before opting in. Use a separate portable profile, back up its timecrs3.arcadedef, and replace only that profile's definition with timecrs3-preview.arcadedef to reproduce. Restore the backup to disable it.

## Validation and outstanding work

On the existing local combined-branch Windows Release build, SpuTest, SpuPcmClockTest and NamcoAcCdvdTest passed (3/3); the GUI built successfully. These checks predate transplanting the changes onto this PR's master base; that exact base has not been rebuilt locally.

A null-renderer VM trace ran for 7500 VBlanks. With the preview guard, all 18,589,696 captured PCM bytes match the original source prefix; the movie counter reaches all 2911 pictures. Without it, the first mismatch occurs at byte 2,392,064 (12.459 seconds). The source has 18,644,992 PCM bytes: the final 55,296 bytes (0.288 seconds) are still untransferred. The tail contains nonzero samples. A complete fix must resolve this truncation and verify real Vulkan/audio synchronization, gameplay and other movies.

The MPS sequence headers indicate 640x320 at 30 fps. Gameplay frame rate and applicability to other titles remain unmeasured. Timing-only tweaks and semaphore-overflow hypotheses were rejected. No ROM, CHD, BIOS, RAM dump, captured media, executable or unrelated Vulkan changes are included.

Compare locally extracted data with:

```sh
python tools/Tc3AudioInvestigation/analyze_tc3_audio.py attract.mps dma0.pcm
```
