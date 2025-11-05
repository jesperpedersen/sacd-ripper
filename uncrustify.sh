#!/bin/bash

function indent()
{
    for f in $1
    do
        uncrustify -c uncrustify.cfg -q --replace --no-backup $f
        rm -f $f.uncrustify
    done
}

indent "src/*.c"
indent "libs/libdstdec/*.c"
indent "libs/libanergistic/*.c"
indent "libs/libsacd/*.c"
indent "libs/libpatchutils/*.c"
indent "libs/libid3/*.c"
indent "libs/libiconv/src/*.c"
indent "libs/libiconv/lib/*.c"
indent "libs/libiconv/tests/*.c"
indent "libs/libiconv/extras/*.c"
indent "libs/libiconv/srclib/*.c"
indent "libs/libiconv/tools/*.c"
indent "libs/libiconv/libcharset/lib/*.c"
indent "libs/libiconv/libcharset/tools/*.c"
indent "libs/libunself/*.c"
indent "libs/libcommon/*.c"
indent "tools/analysis_dump/src/*.c"
indent "tools/sacd_extract/*.c"
