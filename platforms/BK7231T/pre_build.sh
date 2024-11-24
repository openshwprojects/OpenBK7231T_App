DIRNAME=$(dirname $0);
OFS=$IFS
IFS='
'
for X in $(find platforms/BK7231T/override/ -type f);do
S=${X};
D=${X#platforms/BK7231T/override/};
[ -e $D ] && echo "PREBUILD: replacing file\n\t$D\n\twith file\n\t$S" || echo "PREBUILD: adding file\n\t$S\n\tas\n\t$D"
#	cp $S $D;
done
IFS=$OFS
