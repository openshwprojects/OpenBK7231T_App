#!/bin/sh
# Ignore first argument
pydir=$(dirname $0)
shift
for arg in $@; do
    case $arg in
        --prefix|--exec-prefix)
            echo "${pydir}"
            ;;
        --includes)
            echo "-I${pydir}/include"
            ;;
        --libs|--ldflags)
            echo "-L${pydir}/libs -lpython37"
            ;;
    esac
    shift
done
