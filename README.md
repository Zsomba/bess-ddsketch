# DDSketch Measure Module Plugin

The DDSketch Measure Module is a modification of the core Measure Module of BESS. It uses the DDSketch algorythm to store and evaluate collected data of packets.
This module measures the packets' delay and the jitter of the communication.

This package includes two random delay generator modules too.
Uniform Bypass generates delay in seconds from a given interval with equal possibility.
Gaussian Bypass generates delay in seconds with natural distribution with the given mean and deveation.

## Installing into BESS

To be able to use the module the following steps must be followd:

1. Clone bess repository https://github.com/NetSys/bess
2. Copy ddsketch folder into the bess folder
3. Build BESS specifying the inclusion of the desired plugins:
   
    ./build.py --plugin ddsketch --plugin gaussian_bypass --plugin uniform_bypass --plugin delay_registry
    
## Exporting the data

The exporter.py script is used to export the number of buckets and their content to Prometheus.
This script needs the Prometheus Python Client installed.
You can get that from: https://github.com/prometheus/client_python.
To run it, the script has to be in bess's folder and a Pushgateway must be available.
When the Bess daemon is running you just have to run the script from terminal.

You can download Prometheus from https://github.com/prometheus/prometheus and the Pushgateway from https://github.com/prometheus/pushgateway.
For Prometheus to use the Pushgateway as a source, a new target must be added to its config with the path of the Pushgateway.
Grafana is available from https://github.com/grafana/grafana. There are also steps described on how to connect it with Prometheus.
