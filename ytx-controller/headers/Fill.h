#ifndef FILL_H
#define FILL_H

class Fill {
  public: 

    static const int* GetFillPattern(u8 index);
    static const int* ChooseFillPattern();
    static const int* ShufflePattern();

  // private:
    // algorithmic fills
    static int stutter1[STEPS_PER_MEASURE];
    static int stutter2[STEPS_PER_MEASURE];
    static int stutter3[STEPS_PER_MEASURE];
    static int stutter4[STEPS_PER_MEASURE];
    static int half[STEPS_PER_MEASURE];
    static int cut1[STEPS_PER_MEASURE];
    static int cut2[STEPS_PER_MEASURE];
    static int backward1[STEPS_PER_MEASURE];
    static int backward2[STEPS_PER_MEASURE];
    static int drop4[STEPS_PER_MEASURE];
    static int drop8[STEPS_PER_MEASURE];
    static int dropSeq[STEPS_PER_MEASURE];
    static int onlySeq[STEPS_PER_MEASURE];
    static int eightSeq[STEPS_PER_MEASURE];
    static int offbeat1[STEPS_PER_MEASURE];
    static int offbeatRepeat[STEPS_PER_MEASURE];
    static int offbeatSpare[STEPS_PER_MEASURE];
    // static constexpr int stutter1[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0 };
    // static constexpr int stutter2[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, };
    // static constexpr int stutter3[STEPS_PER_MEASURE] = { 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 4, 7, 12, 13, 4, 12 };
    // static constexpr int stutter4[STEPS_PER_MEASURE] = { 2, 3, 4, 5, 10, 11, 12, 13, 2, 3, 4, 1, 10, -1, 12, -1};
    // static constexpr int half[STEPS_PER_MEASURE] = { 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1 };
    // static constexpr int cut1[STEPS_PER_MEASURE] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 0, 1, 0, 1 };
    // static constexpr int cut2[STEPS_PER_MEASURE] = { 0, 2, 4, 6, 0, 2, 4, 6, 8, 10, 12, 14, 8, 10, 12, 14, };
    // static constexpr int backward1[STEPS_PER_MEASURE] = { 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    // static constexpr int backward2[STEPS_PER_MEASURE] = { 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 };
    // static constexpr int drop4[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, -1, -1, -1 };
    // static constexpr int drop8[STEPS_PER_MEASURE] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -1, -1 , -1, -1, -1, -1 };
    // static constexpr int dropSeq[STEPS_PER_MEASURE] = { -1, 1, 2, 3, -1, 5, 6, 7, -1, 9, 10, 11, -1, 13, 14, 15 };
    // static constexpr int onlySeq[STEPS_PER_MEASURE] = { 0, -1, -1, -1, 4, -1, -1, -1, 8, -1, -1, -1, 12, -1, -1, -1 };
    // static constexpr int eightSeq[STEPS_PER_MEASURE] = { 0, -1, 2, -1, 4, -1, 6, -1, 8, -1, 10, -1, 12, -1, 14, -1 };
    // static constexpr int offbeat1[STEPS_PER_MEASURE] = { 4, 1, 2, 3, 12, 5, 6, 7, 4, 9, 10, 11, 12, 13, 14, 15 };
    // static constexpr int offbeatRepeat[STEPS_PER_MEASURE] = { 4, 1, 2, 3, 12, 1, 2, 3, 4, -1, 2, -1, 12, -1, 12, -1 };
    // static constexpr int offbeatSpare[STEPS_PER_MEASURE] = { 4, -1, -1, -1, 4, -1, -1, -1, 4, -1, -1, -1, 4, 4, -1, 0 };
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
