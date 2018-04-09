README for Pd external distribution 'pd-flite'

Last updated for version 0.01

# DESCRIPTION

The 'pd-flite' distribution contains a single Pd external ("flite"),
which provides a high-level text-to-speech interface for English based on
the 'libflite' library by Alan W Black and Kevin A. Lenzo.

Currently tested only under linux.

# REQUIREMENTS

- libflite >= v1.1

    The 'libflite' library by Alan W Black and Keven A. Lenzo
    is required to build the Pd 'flite' external.
    It is available from http://cmuflite.org.

    You may want to apply the patch 'libflite-noaudio.patch'
    which comes with this distribution to the 'libflite'
    sources before compiling them (the '--with-audio=none'
    configure flag did not work for me on its own).

# INSTALLATION

First, build and install the libflite distribution.

Then, issue the following commands to the shell:

    cd PACKAGENAME-X.YY  (or wherever you extracted this distribution)
    make
    make install

# BUILD OPTIONS

The build process allows to specify the voice to use via the
'VOICE' variable (can be one of `kal16`, `kal`, `awb`, `rms`, `slt`).

If you have installed libflite into a non-standard location, you can specify
additional search paths via the `LDFLAGS` and `CPPFLAGS` variables.

Example (using default values):

    make VOICE=kal16 CFLAGS="-I/usr/include/" LDFLAGS="-L/usr/lib/"


For a list of generic variables to tweak, see also:

    make vars

# ACKNOWLEDGEMENTS

Pd by Miller Puckette and others.

Flite run-time speech synthesis library by Alan W Black
and Kevin A. Lenzo.

Ideas, black magic, and other nuggets of information drawn
from code by Guenter Geiger, Larry Troxler, and iohannes m zmoelnig.

# KNOWN BUGS

It gobbles memory, and also processor time on synthesis operations.

No support for alternative voices or lexica, and no
mid- or low-level interface to the libflite functions.

# AUTHOR

Bryan Jurish <moocow@ling.uni-potsdam.de>
