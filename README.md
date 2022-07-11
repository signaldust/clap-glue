# [CLAP](https://github.com/free-audio/clap) glue-code addon for [dust-toolkit](https://github.com/signaldust/dust-toolkit)

**this is very much 'work-in-progress' and has not been properly tested yet**

The basic API glue in `clap-glue.h` and `clap-glue.cpp` is completely self-contained
so you can take these two files and use them without my toolkit just fine.

The basic idea with the low-level wrappers is to simply take the CLAP API as-is
while allowing the plugin to be a proper C++ object (ie. the wrappers take care of
casting the `clap_plugin*` into an actual `this` pointer).

The pointer-tables are generated automatically with template expansion and as long
as the plugin methods are not virtual, the wrapper methods should be zero-cost
(ie. can be inlined or at least turned into direct tail-calls). You can certainly
use virtual methods too, but then they'll have to go through the vtables which is
specifically what this design tries to avoid. See `clap-glue.h` for more details.

The glue also supports multiple plugins in the same binary without having to maintain
any centralized list of plugins (ie. just link them all together and that's it).

---

The rest of it adds some toolkit glue that might become some sort of abstraction
layer in the future.


To use this with dust-toolkit:
```
cd /path/to/dust-toolkit/dust && git clone https://github.com/signaldust/clap-glue.git
```

Then add CLAP header path to your `local.make`:
```
echo 'CFLAGS += -I/path/to/clap/include' >> /path/to/dust-toolkit/local.make
```
