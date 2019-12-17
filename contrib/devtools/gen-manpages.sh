#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

SUPERCOIND=${SUPERCOIND:-$BINDIR/raind}
SUPERCOINCLI=${SUPERCOINCLI:-$BINDIR/rain-cli}
SUPERCOINTX=${SUPERCOINTX:-$BINDIR/rain-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/rain-wallet}
SUPERCOINQT=${SUPERCOINQT:-$BINDIR/qt/rain-qt}

[ ! -x $SUPERCOIND ] && echo "$SUPERCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
TALKVER=($($SUPERCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for raind if --version-string is not set,
# but has different outcomes for rain-qt and rain-cli.
echo "[COPYRIGHT]" > footer.h2m
$SUPERCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $SUPERCOIND $SUPERCOINCLI $SUPERCOINTX $WALLET_TOOL $SUPERCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${TALKVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${TALKVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
