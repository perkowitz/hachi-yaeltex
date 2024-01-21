// Basic flow sequencer module.

#ifndef SCALES_H
#define SCALES_H

// major scale in binary is 1010 1101 0101   (on the 12-tone scale, 1= included in the major scale)
//   reversed is 1010 1011 0101
//   in hex that is ab5
// minor scale is 1011 0101 1010 (natural minor / relative minor)
//   reversed is 0101 1010 1101
//   in hex, 5ad
// OCTA (ancohemitonic symmetric) is 1011 0110 1101; rev = 1011 0110 1101; hex = b6d
// WTF is 1001 1011 0101; backwards 1010 1101 1001; hex ad9
#define MAJOR_SCALE 0xab5
#define MINOR_SCALE 0x5ad
#define OCTATONIC_SCALE 0xb6d
#define WTF_SCALE 0xad9

#define MAJOR_CHORD 0x91        // 1000 1001 0000; r=1001 0001; hex=91
#define MAJOR_6TH_CHORD 0x291   // 1000 1001 0100; r=0010 1001 0001; hex=291
#define MAJOR_7TH_CHORD 0x891   // 1000 1001 0001; r=1000 1001 0001; hex=891
#define MAJOR_AUG_CHORD 0x111   // 1000 1000 1000; r=0001 0001 0001; hex=111
#define MINOR_CHORD 0x89        // 1001 0001 0000; r=1000 1001; hex=89
#define MINOR_6TH_CHORD 0x289   // 1001 0001 0100; r=0010 1000 1001; hex=289
#define MINOR_7TH_CHORD 0x489   // 1001 0001 0010; r=0100 1000 1001; hex=489
#define MINOR_DIM_CHORD 0x49    // 1001 0010 0000; r=0100 1001; hex=49
#define SUS2_CHORD 0x85         // 1010 0001 0000; r=1000 0101; hex=85
#define SUS4_CHORD 0xa1         // 1000 0101 0000; r=1010 0001; hex=a1

// the most notes we might have in a chord, useful for creating arrays of notes if needed
#define MAX_CHORD_NOTES 4

#define MAJOR_THIRD 4
#define MINOR_THIRD 3

#endif
