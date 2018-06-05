# Links
**Presentation** https://docs.google.com/presentation/d/13QHDCdGUv_vvyd4NqX3aTbvmDIk0Qv6Z49Qm97cwqQU/edit?usp=sharing

**Write up** https://docs.google.com/document/d/1gzFwn8OASeHzIqeBpFaDj0erMBB2BO9slvz6Svx8prc/edit?usp=sharing

# Build Instructions

### Running on Mac OS is by far the easiest

You will first need to install and set your system python to python3.5 or otherwise make sure that the python you have in the path is python3.5. We accomplish this using the tool pyenv to manage multiple python versions.

You will need to brew install gcc-8 to build the project as well as a version of cmake > 3.6.

```
git clone <repo> MyPy 
cd MyPy
mkdir -p build 
cd build 
cmake ..
make -j8
```

# Usage
to run a program simply pipe the source code into mypy i.e.
```
cat myprogram.py | ./mypy 
```
or you can pass mypy a file name i.e
```
./mypy myprogram.py
```

Note that mypy only works when executed from the build directory as it invokes helper processes that are found via relative paths to the processes working directory.

# Sources 
 - https://github.com/python/cpython/blob/master/Include/typeslots.h
 - https://tech.blog.aknin.name/tag/block-stack/ 
 - https://docs.python.org/3/reference/ 
 - https://www.ojdip.net/2014/06/simple-jit-compiler-cpp/
 - https://late.am/post/2012/03/26/exploring-python-code-objects.html 
 - https://docs.python.org/2/library/code.html#module-code
 - https://tech.blog.aknin.name/2010/05/26/pythons-innards-pystate/ 
 - http://unpyc.sourceforge.net/Opcodes.html 
 - https://docs.python.org/2/c-api/function.html 
 - https://github.com/jasongros619/Project-Euler 
