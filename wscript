
import os

top = '.'
out = 'build'

ns3_lib_name = "ns3-dev-%s-%s"
""" The name of a ns3 shared library """

def options(opt):
        opt.load('compiler_cxx')
        opt.add_option('--cwd',
                   help=('Set the working directory for a program.'),
                   action="store", type="string", default=None,
                   dest='cwd_launch')

        opt.add_option('--run',
                   help=('Run a locally built program; argument can be a program name,'
                         ' or a command starting with the program name.'),
                   type="string", default='', dest='run')

	opt.add_option('--ns3-path',
                       help='Install path to ns3',
                       action="store", type="string", default=None,
                       dest='ns3_path')

	opt.add_option('--ns3-type',
                       help='The build type used when building ns3 [debug|release]',
                       action="store", type="string", default='debug',
                       dest='ns3_type')

def configure(conf):

	if not conf.options.ns3_path:
            conf.fatal('Please specify a path to ns3')

	ns3_path = os.path.abspath(os.path.expanduser(conf.options.ns3_path))
	
	if not os.path.isdir(ns3_path): 
        	conf.fatal('The specified ns3 path "%s" is not a valid directory' % ns3_path)

	ns3_build = os.path.join(ns3_path, 'build')
     
        if not os.path.isdir(ns3_build): 
		conf.fatal('Could not find the ns3 build directory in "%s"' % ns3_build)

	ns3_type = conf.options.ns3_type
	
	if not ns3_type:
		conf.fatal('You must specify the build type')

        conf.load('compiler_cxx')       
        conf.check_cxx(header_name = 'ns3/core-module.h', includes = [ns3_build])
        conf.check_cxx(lib = ns3_lib_name % ('core', ns3_type), libpath = [ns3_build])

        conf.env['NS3_BUILD'] = [ns3_build]
	conf.env['NS3_TYPE'] = ns3_type

def build(bld):

    ns3_build = bld.env['NS3_BUILD'] 
    ns3_type = bld.env['NS3_TYPE'] 

    ns3_libs = [ns3_lib_name % ('internet', ns3_type),
                ns3_lib_name % ('config-store', ns3_type),
                ns3_lib_name % ('core', ns3_type)]

    ns3_lib_dir = bld.root.find_dir(ns3_build)
    ns3_libs = ns3_lib_dir.ant_glob('*.so')
	
    ns3_libs = [str(lib).split('/')[-1] for lib in ns3_libs]
    ns3_libs = [lib[3:] for lib in ns3_libs]
    ns3_libs = [lib[:-3] for lib in ns3_libs]

    #print ns3_libs
    #exit(0)

    bld.program(source = 'second.cc',
                target = 'second',
                libpath = bld.env['NS3_BUILD'],
                lib = ns3_libs,
                includes = bld.env['NS3_BUILD'],
                features = 'cxx cxxprogram')

                rpath

# the libraries should be in the build folder, so i'm looking a way to add the address, because they are in ns3 folder...but until now, I didn't get anything...this is just # a draf where i'm testing.
