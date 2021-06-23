Demo 1
======

Copyright (c) 2021 Dan Wilcox.

Show greeting for one of the detected languages.

Usage
-----

1. start language identifier
2. run `main.lua` in loaf, ie. drag and drop script onto loaf.app
3. speak

The recording status is shown on screen to the lower right.

Key commands:
* 1-5: show greeting(s) manually
* d: toggle debug view

OSC Communication
-----------------

```
identifier --OSC--> main.lua
```

identifier:
* send address: "localhost"
* send port: 9999

main.lua
* receive port: 9999
