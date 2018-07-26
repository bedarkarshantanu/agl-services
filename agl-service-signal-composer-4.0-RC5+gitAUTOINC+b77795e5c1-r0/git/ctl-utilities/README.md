# Controller Utilities

* Object: Generic Controller Utilities to handle Policy,Small Business Logic, Glue in between components, ...
* Status: Release Candidate
* Author: Fulup Ar Foll fulup@iot.bzh
* Date  : October-2017

## Usage

1) Add ctl-utilities as a submodule to include in your project
```
git submodule add git@github.com:fulup-bzh/ctl-utilities
```

2) Add ctl-utilities as a static library to your binding
```
    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        ctl-utilities
        ... other dependencies ....
    )
```

3) Declare your controller config section in your binding
```
// CtlSectionT syntax:
// key: "section name in config file"
// loadCB: callback to process section
// handle: a void* pass to callback when processing section
static CtlSectionT ctlSections[]= {
    {.key="plugins" , .loadCB= PluginConfig, .handle= &halCallbacks},
    {.key="onload"  , .loadCB= OnloadConfig},
    {.key="halmap"  , .loadCB= MapConfigLoad},
    {.key=NULL}
};

```

3) Do controller config parsing at binding pre-init
```
   // check if config file exist
    const char *dirList= getenv("CTL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    ctlConfig = CtlConfigLoad(dirList, ctlSections);
    if (!ctlConfig) goto OnErrorExit;
```

4) Exec controller config during binding init
```
  int err = CtlConfigExec (ctlConfig);
```

For sample usage look at https://github.com/fulup-bzh/ctl-utilities

