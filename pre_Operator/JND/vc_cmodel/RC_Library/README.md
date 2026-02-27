Picture level RC Library with test environment

## dependency
cvi_float_point

## usage
1. build guide:
 - command: cmake .
 - command: make

2. run guide:
   a. generate test pattern from VC cmodel, run vc command with Rate control related cfg option:
    - RateControl: 1
    - TargetBitrate: 2048000
    - InitialQP: 32
    - sig_pic_rc: 1

   b. get test pattern file (.csv file)
    e.g. Cactus_1920x1080_50_picrc_stats.csv

   b. test RC function and compare result from test pattern file
    ./build/main -f "csv file name" -dir "csv file directory"
    e.g.
     ./build/main -f Cactus_1920x1080_50_picrc_stats.csv -dir $(pwd)
