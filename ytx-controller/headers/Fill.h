#ifndef FILL_H
#define FILL_H

#define CHOOSE_RANDOM_FILL 255
#define FILL_STEP_COUNT 16

class Fill {
  public: 

    static const int* GetFillPattern(u8 index = CHOOSE_RANDOM_FILL);
    static const int* ChooseFillPattern();
    static const int* ShufflePattern();
    static u8 ChooseFillIndex();
    static int MapFillPattern(u8 fillPattern, u8 currentStep);

  // private:
    // algorithmic fills
    static int stutter1[FILL_STEP_COUNT];
    static int stutter2[FILL_STEP_COUNT];
    static int stutter3[FILL_STEP_COUNT];
    static int stutter4[FILL_STEP_COUNT];
    static int half[FILL_STEP_COUNT];
    static int cut1[FILL_STEP_COUNT];
    static int cut2[FILL_STEP_COUNT];
    static int backward1[FILL_STEP_COUNT];
    static int backward2[FILL_STEP_COUNT];
    static int drop4[FILL_STEP_COUNT];
    static int drop8[FILL_STEP_COUNT];
    static int dropSeq[FILL_STEP_COUNT];
    static int onlySeq[FILL_STEP_COUNT];
    static int eightSeq[FILL_STEP_COUNT];
    static int offbeat1[FILL_STEP_COUNT];
    static int offbeatRepeat[FILL_STEP_COUNT];
    static int offbeatSpare[FILL_STEP_COUNT];

    #define FILL_PATTERN_COUNT 17
        static const int* fills[FILL_PATTERN_COUNT];
    // static constexpr const int* fills[FILL_PATTERN_COUNT] = { 
    //         stutter1, stutter2, stutter3, stutter4, 
    //         half, cut1, cut2, 
    //         backward1, backward2, 
    //         drop4, drop8, dropSeq, onlySeq, eightSeq, 
    //         offbeat1, offbeatRepeat, offbeatSpare 
    //       };

};

#endif
