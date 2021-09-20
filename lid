#! /bin/sh
#
# LanguageIdentifier wrapper script
#
# Copyright (c) 2021 ZKM | Hertz-Lab
# Dan Wilcox <dan.wilcox@zkm.de>
#
# BSD Simplified License.
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "LICENSE.txt," in this distribution.
#
# This code has been developed at ZKM | Hertz-Lab as part of „The Intelligent
# Museum“ generously funded by the German Federal Cultural Foundation.

LID=LanguageIdentifier
DIR="$(dirname $0)/bin"
EXEC=${DIR}/${LID}

# platform specifics
case "$(uname -s)" in
	Linux*)  ;;
	Darwin*)
		# invoke executable inside .app bundle
		EXEC=$DIR/${LID}.app/Contents/MacOS/${LID}
		;;
	CYGWIN*) ;;
	MINGW*)  ;;
	*)       ;;
esac

# TODO: add lib paths for Linux?
# go
$EXEC $@
