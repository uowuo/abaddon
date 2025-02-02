# Working with translations

Translators must first ensure that the tools coming with the gettext package are
installed. If that's not the case install it using your package manager.

```sh
# On Debian/Ubuntu systems. Use your package manager
sudo apt install gettext
```

## Adding new translation files

Adding new translation files is as easy as issuing a command. In the command
below, replace `LL_CC` for the country code you are translating the project to
and set an appropriate encoding to the country code.

```sh
msginit -i po/abaddon.pot -o po/LL_CC.po --locale LL_CC[.ENCODING]
```

After that, you'll have to append the country code to `PO_NAMES` variable in
[CMakeLists.txt](CMakeLists.txt), build file responsible for compiling and
updating the translations.

```cmake
# Insert the country code in alphabetical order.
# Replace LL_CC for the country code
list(APPEND PO_NAMES pt_BR LL_CC)
```

Then, compile the translation file by simply issuing the build command within
the build directory. Doing this won't only compile but it'll also update the .po
file in case there is any modification made to abaddon.pot.

```sh
cmake --build build-dir/
```

To ensure that everything is in place, run the program with `LANGUAGE`
environment variable set to the country code you are providing a translation
for:

```sh
# This snippet assumes that the current directory is the build directory
LANGUAGE=pt_BR ./abaddon
```

## Adding new strings to be translated

When expanding the software, there might be some text that will be shown to
the user (this does not include log messages as these are meant to be read
by any developer) that needs to be translated as well. Marking a string for
translation is as easy as closing the target string with `_(<your_string>)`.
It may look like this

```cpp
some_label->set_text(_("I will appear in the user's native language"))
```

Make sure that the file where the marked strings are is registered in the
**POTFILES** file, so the main pot file can be updated by calling gettext
functions. After that, the main .pot file has to be updated so the
remaining translation files can be synchronised with the new strings.

```sh
    xgettext -f po/POTFILES -o po/abaddon.pot --from-code=UTF-8 --keyword=_
    cmake --build build_dir/
```
