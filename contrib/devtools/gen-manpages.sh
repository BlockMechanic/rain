#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

RAIND=${RAIND:-$BINDIR/raind}
RAINCLI=${RAINCLI:-$BINDIR/rain-cli}
RAINTX=${RAINTX:-$BINDIR/rain-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/rain-wallet}
RAINQT=${RAINQT:-$BINDIR/qt/rain-qt}

[ ! -x $RAIND ] && echo "$RAIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
RAINVER=($($RAINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for raind if --version-string is not set,
# but has different outcomes for rain-qt and rain-cli.
echo "[COPYRIGHT]" > footer.h2m
$RAIND --version | sed -n '1!p' >> footer.h2m

for cmd in $RAIND $RAINCLI $RAINTX $WALLET_TOOL $RAINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${RAINVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${RAINVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
