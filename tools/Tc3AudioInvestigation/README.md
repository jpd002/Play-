# Japanese TC3 movie audio bank and end handling

The tst1vera movie loader can reuse an EE bank before audio releases its header. Packet 10's 14-block length becomes packet 12's 17-block length while the audio index is still 14. The patch in arcadedefs/timecrs3.arcadedef waits for bank release while pumping audio, then also drains the final packet and waits for the IOP half-buffer transition before the movie's normal end handler. It changes no playback frequency.

The patch is specific to the Japanese tst1vera executable. It redirects read submission at 0x14c540 and movie-end handling at 0x14a984 to stubs at 0x000f0000..0x000f073c. This area was zero in the local RAM dump and is outside this HLE kernel's state/stack area; this is a game-specific workaround, not a general allocation mechanism. The copied audio-pump fragments preserve the original calling convention, omit video/queue advancement, and prevent a final bank switch.

## Measurements

With the earlier bank guard alone, the source-matching capture stopped at 18,589,696 bytes, leaving 55,296 bytes (0.288 s). Waiting for video-end alone stalled further transfers; draining the packet alone still left 14,336 bytes in flight. The final patch waits for the IOP buffer transition as well.

The final 7500-VBlank null-renderer trace contains all 18,644,992 source PCM bytes exactly, with 2048 additional bytes of DMA read-ahead before stopping. The additional capture is not part of the source movie and is reported separately by the comparison script. A resampled comparison of the final 0.5 s of source audio with the mixed WAV finds correlation 0.99175 and a tail endpoint at capture time 121.896 s, confirming the source tail also reaches mixed output. Correlation is not bit-identical resampling validation.

Playback starts within one VBlank: counters and first DMA become active between VBlanks 1487 and 1488. No 0.288 s initial delay was observed. Video counter reaches all 2911 pictures. Host monitor presentation and Windows device latency were not measured.

The existing local combined-branch Windows Release build passes NamcoAcCdvdTest, SpuPcmClockTest and SpuTest. The core corrections preserve PCM fractional phase and fix sector indexing; neither alone fixes the TC3 bank/end race. The master-based PR transplant has not been rebuilt locally. Actual Vulkan synchronization, gameplay and other movies remain to be verified. No general fix for other games is claimed.

No ROM, BIOS, CHD, RAM dump, media capture or executable is distributed. timecrs3-preview.arcadedef mirrors the installed definition for portable-profile comparisons. To disable the workaround, restore the previous empty patches array.

Compare local inputs: python tools/Tc3AudioInvestigation/analyze_tc3_audio.py attract.mps dma0.pcm
