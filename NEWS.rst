News for kodo-ns3-examples
==========================

This file lists the major changes between versions. For a more
detailed list of every change, see the Git log.

Latest
------
* Major: Use new kodo repository.

3.0.0
-----
* Minor: Upgrade to the ns-3.30 release
* Major: Use the --destdir option to specify the target installation folder
  for the examples (e.g. ``~/ns-3-dev/examples/kodo``).
  Consequently, the --ns3_path option was removed.
* Major: Upgrade to kodo-rlnc 15

2.0.0
-----
* Minor: Added the kodo-wifi-broadcast-object example that transmits a
  large object in memory which is automatically split into multiple
  blocks/generations.
* Minor: Changed the wifi example to use the RandomPropagationLossModel
  which allows us to simulate randomized packet loss on the receivers.
  With the default loss values, the receivers randomly drop 50% of the packets.
* Major: Remove the kodo-cpp dependency, and use kodo-rlnc 13 directly in the
  examples. With this change, the users can leverage the full public API of
  kodo-rlnc in the ns-3 simulations.

1.0.0
-----
* Major: Initial release
