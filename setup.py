#!/usr/bin/env python

import os, os.path, sys, shutil, commands, fnmatch, glob
from distutils.core import setup,Extension
from distutils.command.build_ext import build_ext
from distutils.sysconfig import *
from distutils.command.install import install
from distutils.command.install_data import install_data

py_includes = get_python_inc(plat_specific=1)
py_libs =  get_python_lib(plat_specific=1, standard_lib = 1)
x_libraries = 'X11'
pythonlib = get_python_lib(plat_specific=1)
pythoninc = get_python_inc()
ver = get_python_version()
pythonver = 'python' + ver

add_lib_dirs = []
add_inc_dirs = []
add_inc_dirs.append(py_includes)
add_lib_dirs.append(py_libs)

def find_x(xdir=""):
    if xdir != "":
        add_lib_dirs.append(os.path.join(xdir,'lib64'))
        add_lib_dirs.append(os.path.join(xdir,'lib'))
        add_inc_dirs.append(os.path.join(xdir,'include'))
    elif sys.platform == 'darwin' or sys.platform.startswith('linux'):
        add_lib_dirs.append('/usr/X11R6/lib64')
        add_lib_dirs.append('/usr/X11R6/lib')
        add_inc_dirs.append('/usr/X11R6/include')

TOUCHPY_EXTENSIONS = [Extension('touchpy/xutilmodule', ['xutil.c'],
                              include_dirs=add_inc_dirs,
                              library_dirs=add_lib_dirs,
                              libraries = [x_libraries])]


args = sys.argv[:]
for a in args:
    if a.startswith("--local="):
         """Adds a command line option --local=<install-dir> which is an abbreviation for
         'put all of pyraf in <install-dir>/pyraf'."""
         dir = os.path.abspath(a.split("=")[1])
         sys.argv.extend([
                "--install-lib="+dir,
                "--install-scripts=%s" % os.path.join(dir,"pyraf"),
                ])
         sys.argv.remove(a)
         args.remove(a)


#TOUCHPY_CLCACHE_DIR = os.path.join('pyraf', 'clcache')
#DATA_FILES = [('pyraf', PYRAF_DATA_FILES), (PYRAF_CLCACHE_DIR, PYRAF_CLCACHE)]
DATA_FILES = []
class smart_install_data(install_data):
    def run(self):
        #need to change self.install_dir to the library dir
        install_cmd = self.get_finalized_command('install')
        self.install_dir = getattr(install_cmd, 'install_lib')
        return install_data.run(self)


def dosetup():
    r = setup(name = "TouchPy",
              version = "1.0",
              description = "A Python/touchlib based touchscreen interface",
              author = "Goran Medakovic",
              maintainer_email = "goran.it@gmail.com",
              url = "http://www.microlink.co.yu/touchpy",
              license = "http://www.microlink.co.yu/touchpy/LICENSE",
              platforms = ["unix"],
              packages = ['touchpy'],
              package_dir = {'touchpy':'lib'},
              cmdclass = {'install_data':smart_install_data},
              data_files = DATA_FILES,
              #scripts = ['lib/touchpy'],
              ext_modules = TOUCHPY_EXTENSIONS)
    
    return r


def main():
    args = sys.argv[2:]
    x_dir = ""
    for a in args:
        if a.startswith('--with-x='):
            x_dir = a.split("=")[1]
            sys.argv.remove(a)
    find_x(x_dir)
    dosetup()

if __name__ == "__main__":
    main()
