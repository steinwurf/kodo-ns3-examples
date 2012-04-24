In order to make a simmulation of our code out of ns3, some steps must be follow. As it can be observed in the files, waf is used to configure and build the program.
In this case, it is going to be simulated the code Adhoc. However, firstly it is needed to make the configuration of waf and specified the path where the libraries and h files are placed. 
::
  ./waf configure --ns3-path=../../repo/ns-3-allinone/ns-3-dev/ --ns3-type=debug

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



Before execute the program, it would be required to create symbolic links of the ns3 libraries in our /usr/lib. For that, it is written the next sentence in the terminal :
::
  sudo ln -s /path/to/ns3/build/*-debug.so /usr/lib/

And finally, executing the program:
::
  ./build/adhoc --N=3 --N2=6

 Where it is specified the number of simulations and the number of nodes used durin this simulations, respectively.
Moreover other parameters can be fixed as:

  --PrintHelp: Print this help message.

  --PrintGroups: Print the list of groups.

  --PrintTypeIds: Print all TypeIds.

  --PrintGroup=[group]: Print all TypeIds of group.

  --PrintAttributes=[typeid]: Print all attributes of typeid.

  --PrintGlobals: Print the list of globals.

  User Arguments:

    --N: set run number of simulation

    --N2: number of nodes

    --phyMode: Wifi Phy mode

    --rss: received signal strength

    --packetSize: size of application packet sent

    --numPackets: number of packets generated

    --interval: interval (seconds) between packets

    --verbose: turn on all WifiNetDevice log components



After that, the result of the progrma have to be displayed in the screen.


