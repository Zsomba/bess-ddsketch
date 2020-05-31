from __future__ import print_function
import scapy.all as scapy
import sys
from test_utils import *


class BessDDSketchTest(BessModuleTestCase):

    def gen_packet(self, dst_ip="0.0.0.0"):
        eth = scapy.Ether(src='02:1e:67:9f:4d:ae', dst='06:16:3e:1b:72:32')
        ip = scapy.IP(src='1.2.3.4', dst=dst_ip)
        udp = scapy.UDP(sport=10001, dport=10002)
        payload = ('hello' + '0123456789' * 200)[:60 - len(eth / ip / udp)]
        pkt = eth / ip / udp / payload

        return bytes(pkt)

    def test_ddsketch_data(self):
        packets = [
            self.gen_packet(dst_ip='172.16.100.1'),
            self.gen_packet(dst_ip='0.0.0.0'),
            self.gen_packet(dst_ip='216.135.20.0')
        ]

        measure = Measure(latency_ns_resolution=1, latency_ns_max=100000)

        Source() -> Rewrite(templates=packets) -> Timestamp() -> measure -> Sink()

        self.bess.resume_all()
        # Run for .5 seconds to warm up ...
        time.sleep(0.5)
        # ... then clear accumulated stats.
        self.bess.run_module_command(measure.name, 'clear', 'EmptyArg', {})
        # Run for 2 more seconds to accumulate real stats.
        time.sleep(2)
        self.bess.pause_all()
        self.assertBessAlive()

        pct = [0, 25, 50, 99, 100]
        stats = self.bess.run_module_command(
            measure.name,
            'get_summary',
            'MeasureCommandGetSummaryArg',
            {'latency_percentiles': pct}
        )
        print()
        print(stats)


suite = unittest.TestLoader().loadTestsFromTestCase(BessDDSketchTest)
results = unittest.TextTestRunner(verbosity=2).run(suite)

if results.failures or results.errors:
    sys.exit(1)


