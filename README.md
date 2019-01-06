# ptreefs
> Process tree filesystem for Linux

ptreefs is a modified version of the Linux kernel ([v4.4](https://elixir.bootlin.com/linux/v4.4/source) to be specific) that includes a new filesystem, *ptreefs*, which builds a directory containing information on the current state of the process tree.

Here's a brief explanation of how the filesystem is structured:
- The root directory contains a folder named using the PID of the oldest still-running process
- That folder will contain a file with the name of the process corresponding with that PID (**note**: this file will be suffixed with *.name*). 
- That folder will also contain folders named by the PIDs of each of that processes' children

Features:
- ptreefs is updated accordingly when processes are created/terminated
- This filesystem is in-memory (much like [ramfs](https://wiki.debian.org/ramfs) or [procfs](https://en.wikipedia.org/wiki/Procfs)), so data is not placed in persistent storage

![](etc/screencap.gif)

## Installation

This version of the Linux kernel can be compiled using the following commands.

**Note**: Use 2x the number of available cores to your machine when using the -j option. (e.g. on a dual core machine run `make -j4`)

```sh
$make -j6
$sudo make modules_install && sudo make install
```

Now that the kernel is compiled and the boot image has been added, reboot your machine and boot from the new image.

## Testing

The testing suite for ptreefs can be found under the `user` directory. 

Included is a test file `ptreeps.c` and its corresponding Makefile. This script fork several processes and terminate those processes, printing out the process tree at each stage.

## Meta

Distributed under the GNU GPL v3.0 license. See ``LICENSE`` for more information.

## Contributing

1. [Fork](https://github.com/matthewachan/ptreefs/fork) the repo
2. Create a feature branch (e.g. `git checkout -b feature/new_feature`)
3. Add & commit your changes
4. Check your code using the Linux checkpatch (kernel/scripts/checkpatch.pl)
5. Push to your feature branch (e.g. `git push origin feature/new_feature`)
6. Create a new pull request
