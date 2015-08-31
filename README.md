#Junior-
---

Hello. Currently this is a project in the works.

Junior- is a dialect of Lisp. I am making this project to understand
more of the C language, learn how to program my own language, and learn more
about functional languages.

###How to run
---
If you are using Windows, the behaviour of the ```editline``` library is added
by default.

If you are using a Mac, then the ```editline``` library already  comes with
your command line tools. If, however, that you receive an error based on
this library, remove the line
```c
#include <editline/history.h>
```
This should now work.

On a GNU/Linux (Linux) machine, you can install the editline
library by using your package management tool
 (eg, ```sudo apt-get``` in Debian/Ubuntu), and typing
```install libedit-dev```. This will get you up and running with editline.

####Compiling
Currently, the method of compiling the current program is as follows:

If you are using a Unix/Linux machine (OS X is Unix), then run the command
```shell
cc -std=c99 -Wall junior.c mpc.c l-edit -lm -o junior
```

On a Windows,
```shell
cc -std=c99 -Wal junior.c mpc.c -o junior
```

Please be aware, you can change the executable to any name you'd like. However,
the parameters given to the C Compiler (cc) must be added (which are OS-specific)
in order to be compiled properly.

###Credit
---
This project is made possible through [Daniel Holden (orangeduck)](https://github.com/orangeduck)
for both writing the book [Build Your Own Lisp](http://buildyourownlisp.com/)
as well as providing the MPC library which makes it very to easy to do very
cool things in C.
