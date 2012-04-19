As a starting point, it is going to be created a folder called test-ns3 where it is going to be download the waf code :
wget http://waf.googlecode.com/files/waf-1.6.11.tar.bz2
tar xjvf waf-1.6.11.tar.bz2
cd waf-1.6.11
python waf-light
The result of this actions will be something like:
Configuring the project
’build’ finished successfully (0.001s)
Checking for program python
: /usr/bin/python
Checking for python version
: (2, 6, 5, ’final’, 0)
’configure’ finished successfully (0.176s)
Waf: Entering directory ‘/waf-1.6.11/build’
[1/1] create_waf: -> waf
Waf: Leaving directory ‘/waf-1.6.11/build’
’build’ finished successfully (2.050s)
Changinng the permits of waf folder and execute:
$ chmod 755 waf
$ ./waf --version
The result will be something like:
waf 1.6.11 (a769d6b81b04729804754c4d5214da063779a65)
At this moment, it is needed a code to be simulated. In this first case, it has been
chosen the code second.cc which is placed in the path /path/to/ns3/examples/tutorials,
and it is going to be copied into the folder test-ns3.
./waf configure --cwdl=../../kodo/--ns3-path=../../repo/ns-3-allinone/ns-3-dev/ --ns3-type=debug
Where there are specified the NS3 path, the kodo path and the NS3 library type in this system. These values
will be changed for each user in his own system, adding his own NS3 path, kodo path and type.
The result of this action would be something like that:
Setting top to
: /home/edwin/waf_examples/test-ns3.2
Setting out to
: /home/edwin/waf_examples/test-ns3.2/build
Checking for ’g++’ (c++ compiler)
: /usr/bin/g++
’configure’ finished successfully (0.072s)
Next step, build the program with this sentence:
./waf
The result of this action would be something like that:
Waf: Entering directory ‘/home/edwin/waf_examples/test-ns3.2/build’
[1/2] cxx: second.cc -> build/second.cc.1.o
[2/2] cxxprogram: build/second.cc.1.o -> build/second
Waf: Leaving directory ‘/home/edwin/waf_examples/test-ns3.2/build’
’build’ finished successfully (3.122s)
Before execute the program, it would be required to create symbolic links of the
ns3 libraries in our /usr/lib. For that, it is written the next sentence in the terminal :
sudo ln -s /path/to/ns3/build/*-debug.so /usr/lib/
And finally, executing the program:
./build/adhoc_1 10 10
(We are inside the folder test-ns3).

