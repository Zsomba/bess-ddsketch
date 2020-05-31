from prometheus_client import Gauge, push_to_gateway, CollectorRegistry
import os
import sys
import time

script_dir = os.path.realpath(os.path.dirname(__file__))
pybess_path = os.path.join(script_dir, "pybess")
utils_path = os.path.join(script_dir, "bessctl")

sys.path.append(pybess_path)
sys.path.append(utils_path)

from pybess import bess


def get_content(module_name, metric):
    """
    Exports the content of the DDSketch instance
    :param module_name: the name of the DDSketch instance
    :param metric: the metric to store the data
    :return: None
    """
    buckets = runtime.run_module_command(
        module_name,
        'get_content',
        'DDSketchCommandGetContentArg',
        {}
    )

    for bucket in buckets.content:
        metric.labels(bucket_index=bucket.index).set(bucket.counter)


def check_bucket_count(module_name, counter):
    """
    Exports the number of buckets of the DDSketch instance
    :param module_name: the name of the DDSketch instance
    :param counter: the metric to store the data
    :return: None
    """
    stats = runtime.run_module_command(
        module_name,
        'get_stat',
        'DDSketchCommandGetStatArg',
        {}
    )

    counter.set(stats.bucket_number)


if __name__ == '__main__':

    runtime = bess.BESS()
    try:
        runtime.connect()
    except bess.BESS.APIError:
        raise Exception('BESS is not running')

    modules = runtime.list_modules()
    name = None
    for module in modules.modules:
        if module.mclass == "DDSketch":
            name = module.name

    if not name:
        raise Exception("There is no DDSketch module in the pipeline.")

    registry = CollectorRegistry()
    buckets = Gauge(
        'ddsketch_buckets',
        "The content of the DDSketch's buckets",
        ['bucket_index'],
        registry=registry
    )

    bucket_number = Gauge(
        'ddsketch_bucket_count',
        "The number of buckets currently in use by DDSketch",
        registry=registry
    )

    # This loop collects the data from the DDSketch instance
    # and sends to the Pushgateway in every second
    while runtime.is_connected() and not runtime.is_connection_broken():
        get_content(module_name=name, metric=buckets)
        check_bucket_count(module_name=name, counter=bucket_number)
        push_to_gateway(gateway='localhost:9091', job='ddsketch', registry=registry)
        time.sleep(1)
