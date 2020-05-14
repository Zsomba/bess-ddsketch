# DDSketch Measure Module Plugin

The DDSketch Measure Module is a modification of the core Measure Module of BESS. It uses the DDSketch algorythm to store and evaluate collected data of packets.
This module measures the packets' delay and the jitter of the communication.

## Installing into BESS

To be able to use the module the following steps must be followd:

1. Copy ddsketch folder into the bess folder
2. Build BESS specifying the inclusion of the ddsketch plugin:
   
    ./build.py --plugin ddsketch