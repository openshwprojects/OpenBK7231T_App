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
            echo "-I${pydir}/include/python3.7m"
            ;;
        --libs|--ldflags)
            echo "-L${pydir}/lib/python3.7/config-3.7m-x86_64-linux-gnu -L${pydir}/lib -lpython3.7m -lcrypt -lpthread -ldl  -lutil -lrt -lm"
            ;;
    esac
    shift
done
