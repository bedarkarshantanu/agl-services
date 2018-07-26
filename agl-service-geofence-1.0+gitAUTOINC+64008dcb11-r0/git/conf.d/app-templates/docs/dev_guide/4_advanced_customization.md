# Advanced customization

## Including additionnals cmake files

Advanced tuning is possible using addionnals cmake files that are included
automatically from some specifics locations. They are included in that order:

- Project CMake files normaly located in _<project-root-path>/conf.d/app-templates/cmake/cmake.d_
- Home CMake files located in _$HOME/.config/app-templates/cmake.d_
- System CMake files located in _/etc/app-templates/cmake.d_

CMake files has to be named using the following convention: `XX-***.cmake`,
where `XX` are numbers, `***` file name (ie. `99-my_customs.cmake`).

So, saying that you should be aware that every normal cmake variables used at
project level could be overwrited by home or system located cmake files if
variables got the same name. Exceptions are cached variables set using
**CACHE** keyword:

Example:

```cmake
set(VARIABLE_NAME 'value string random' CACHE STRING 'docstring')
```

## Include customs templated scripts

As well as for additionnals cmake files you can include your own templated
scripts that will be passed to cmake command `configure_file`.

Just create your own script to the following directories:

- Home location in _$HOME/.config/app-templates/scripts_
- System location in _/etc/app-templates/scripts_

Scripts only needs to use the extension `.in` to be parsed and configured by
CMake command.
