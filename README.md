# DDSketch Measure Module Plugin

The DDSketch Measure Module is a modification of the core Measure Module of BESS. It uses the DDSketch algorythm to store and evaluate collected data of packets.
This module measures the packets' delay and the jitter of the communication.

## Installing into BESS

To be able to use the module the following steps must be followd:

1. Clone bess repository https://github.com/NetSys/bess
2. Copy ddsketch folder into the bess folder
3. Build BESS specifying the inclusion of the desired plugins:
   
    ./build.py --plugin ddsketch --plugin gaussian_bypass --plugin uniform_bypass
