# AFL module for kcov Framework
Simple module that exposes to the User Space AFL like SHM region that can be consumed by modified AFL.    
This module demonstrate how to write own extension to the NetBSD [kcov(4)](https://netbsd.gw.com/cgi-bin/man-cgi?kcov++NetBSD-current).

## Compilation
Compilation require debugcon_printf, can be downloaded [from here](https://github.com/krytarowski/debugcon_printf)

```
# Compile kernel module
make S=/~/workspace/mybsd/src/sys

# Compile user space stub 
gcc ./main.c -o afl_test
```
