
# Quickstart

## Initialization

To use these templates files on your project just install the reference files using
**git submodule** then use `config.cmake` file to configure your project specificities :

```bash
git submodule add https://gerrit.automotivelinux.org/gerrit/apps/app-templatesconf.d/app-templates conf.d/app-templates
mkdir conf.d/cmake
cp conf.d/app-templates/cmake/config.cmake.sample conf.d/cmake/config.cmake
```

Edit the copied config.cmake file to fit your needs.

Now, create your top CMakeLists.txt file which include `config.cmake` file.

An example is available in **app-templates** submodule that you can copy and
use:

```bash
cp conf.d/app-templates/cmake/CMakeLists.txt CMakeLists.txt
```

## Create your CMake targets

For each target part of your project, you need to use ***PROJECT_TARGET_ADD***
to include this target to your project.

Using it, make available the cmake variable ***TARGET_NAME*** until the next
***PROJECT_TARGET_ADD*** is invoked with a new target name. 

So, typical usage defining a target is:

```cmake
PROJECT_TARGET_ADD(SuperExampleName) --> Adding target to your project

add_executable/add_library(${TARGET_NAME}.... --> defining your target sources

SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES.... --> fit target properties
for macros usage

INSTALL(TARGETS ${TARGET_NAME}....
```

## Targets PROPERTIES

You should set properties on your targets that will be used to package your
apps in a widget file that could be installed on an AGL system.

Specify what is the type of your targets that you want to be included in the
widget package with the property **LABELS**:

Choose between:

- **BINDING**: Shared library that be loaded by the AGL Application Framework
- **HTDOCS**: Root directory of a web app
- **DATA**: Resources used by your application
- **EXECUTABLE**: Entry point of your application executed by the AGL
 Application Framework

```cmake
SET_TARGET_PROPERTIES(${TARGET_NAME}
	PREFIX "afb-"
	LABELS "BINDING"
	OUTPUT_NAME "file_output_name")
```

> **TIP** you should use the prefix _afb-_ with your **BINDING* targets which
> stand for **Application Framework Binding**.
