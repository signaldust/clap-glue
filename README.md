# CLAP glue-code addon for dust-toolkit

To use this with [dust-toolkit](https://github.com/signaldust/dust-toolkit):
```
cd /path/to/dust-toolkit/dust && git clone https://github.com/signaldust/clap-glue.git
```

Then add [CLAP](https://github.com/free-audio/clap) header path to your `local.make`:
```
echo 'CFLAGS += -I/path/to/clap/include' >> /path/to/dust-toolkit/local.make
```

The basic API glue in `clap-glue.h` and `clap-glue.cpp` does not rely on anything in dust-toolkit so can also be used standalone.

