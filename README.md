# xrop-esp32

xrop-esp32 is a clone of https://github.com/jsandin/xrop, where jsandin added support for ESP8266 for xrop. Original xrop repo is https://github.com/acama/xrop.

I fixed jsandin/xrop, and created this repo. Fixes include
- made it compile-able
- make so that it does not segfault on start
- select all executable segments of elf file, not just vectors
- fix calculation of ROPchain addresses
- re-added plain output



## Build Instructions
```
make
```


## How to use

```
./xrop ../mongoose-os/myfirstApp/build/objs/fw.elf 
```

