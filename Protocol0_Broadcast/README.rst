In order to make a simulation of our code out of ns3, some steps must be follow. As it can be observed in the files, waf is used to configure and build the program.
In this case, it is going to be simulated the code Adhoc. However, firstly it is needed to make the configuration of waf and specified the path where the libraries and h files are placed. 
::
  ./waf configure--bundle=ALL --bundle-path=~/bundle_dependencies/ --ns3-path=~/repo/ns-3-allinone/ns-3-dev/ --ns3-type=debug

These values will be changed for each user in his own system, adding his own NS3 path and type.
The result of this action would be something like that:
::
  Setting top to
  : /home/edwin/waf_examples/test-ns3.2
  Setting out to
  : /home/edwin/waf_examples/test-ns3.2/build
  Checking for ’g++’ (c++ compiler)
  : /usr/bin/g++
  ’configure’ finished successfully (0.072s)

Next step, build the program with this sentence:
::
  ./waf

The result of this action would be something like that:
::
  Waf: Entering directory ‘/home/edwin/waf_examples/test-ns3.2/build’
  [1/4] cxx: adhoc_1.cc -> build/adhoc_1.cc.1.o
  [2/4] cxx: pep-wifi-net-device_1.cc -> build/pep-wifi-net-device_1.cc.1.o
  [3/4] cxx: code-header.cc -> build/code-header.cc.1.o
  [4/4] cxxprogram: build/adhoc_1.cc.1.o build/pep-wifi-net-device_1.cc.1.o build/code-header.cc.1.o -> build/adhoc_1
  Waf: Leaving directory `/home/edwin/waf_examples/test-ns3.5/test-ns3.4/build'
  'build' finished successfully (16.052s)



And finally, executing the program:
::
  ./build/linux/adhoc --rss=-80 --RelayActivity=50 --numPackets=2000 --EnableCode=1 --ChecksumEnabled=true --Symbols=5

 Where it is specified some parameters. This list content all the parameters that can be fixed in this way:

  --PrintHelp: Print this help message.

  --PrintGroups: Print the list of groups.

  --PrintTypeIds: Print all TypeIds.

  --PrintGroup=[group]: Print all TypeIds of group.

  --PrintAttributes=[typeid]: Print all attributes of typeid.

  --PrintGlobals: Print the list of globals.

  User Arguments:

     --RelayActivity: Number of symbols in each generation

    --EnableCode: enable ncoding 1, disable coding 0

    --Symbols: Number of symbols in each generation

    --EnableRencode: enable rencoding 1, disable rencoding 0

    --phyMode: Wifi Phy mode

    --rss: received signal strength

    --packetSize: size of application packet sent

    --numPackets: number of packets generated

    --interval: interval (seconds) between packets

    --verbose: turn on all WifiNetDevice log components

    --distance: distance 

    --seed: seed 



After that, the result of the program have to be displayed in the screen.


