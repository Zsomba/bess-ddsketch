# DDSketch Measure Module

The DDSketch Measure Module is a modification of the core Measure Module of BESS. It uses the DDSketch algorythm to store and evaluate collected data of packets.
This module measures the packets' delay and the jitter of the communication.

## Installing into BESS

To be able to use the module the following steps must be followd:

1. Copy ddsketch.h and ddsketch.cc files into the core/modules folder of bess
2. Extend the module_msg.proto file found under protobuf with the DDSketch related messages found at the end of the module_msg.proto file in the repo
3. Rebuild BESS