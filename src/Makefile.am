# Copyright (c) 2013-2016 The Rain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%:
	@echo '$*' = '$($*)'

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

RAIN_INCLUDES=-I$(builddir) -I$(srcdir)/secp256k1/include $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

RAIN_INCLUDES += $(UNIVALUE_CFLAGS)

LIBRAIN_SERVER=librain_server.a
LIBRAIN_COMMON=librain_common.a
LIBRAIN_CONSENSUS=librain_consensus.a
LIBRAIN_CLI=librain_cli.a
LIBRAIN_UTIL=librain_util.a
LIBRAIN_CRYPTO_BASE=crypto/librain_crypto_base.a
LIBRAINQT=qt/librainqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBRAIN_SMSG=librain_smsg.a

if ENABLE_ZMQ
LIBRAIN_ZMQ=librain_zmq.a
endif
if BUILD_RAIN_LIBS
LIBRAINCONSENSUS=librainconsensus.la
endif
if ENABLE_WALLET
LIBRAIN_WALLET=librain_wallet.a
LIBRAIN_WALLET_TOOL=librain_wallet_tool.a
endif

LIBRAIN_CRYPTO= $(LIBRAIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBRAIN_CRYPTO_SSE41 = crypto/librain_crypto_sse41.a
LIBRAIN_CRYPTO += $(LIBRAIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBRAIN_CRYPTO_AVX2 = crypto/librain_crypto_avx2.a
LIBRAIN_CRYPTO += $(LIBRAIN_CRYPTO_AVX2)
endif
if ENABLE_SHANI
LIBRAIN_CRYPTO_SHANI = crypto/librain_crypto_shani.a
LIBRAIN_CRYPTO += $(LIBRAIN_CRYPTO_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBRAIN_CRYPTO) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_COMMON) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_SERVER) \
  $(LIBRAIN_CLI) \
  $(LIBRAIN_WALLET) \
  $(LIBRAIN_WALLET_TOOL) \
  $(LIBRAIN_SMSG) \
  $(LIBRAIN_ZMQ)

lib_LTLIBRARIES = $(LIBRAINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_RAIND
  bin_PROGRAMS += raind
endif

if BUILD_RAIN_NODE
  bin_PROGRAMS += rain-node
endif

if BUILD_RAIN_CLI
  bin_PROGRAMS += rain-cli
endif

if BUILD_RAIN_TX
  bin_PROGRAMS += rain-tx
endif

if ENABLE_WALLET
if BUILD_RAIN_WALLET
  bin_PROGRAMS += rain-wallet
endif
endif

if BUILD_RAIN_UTIL
  bin_PROGRAMS += rain-util
endif

.PHONY: FORCE check-symbols check-security
# rain core #
RAIN_CORE_H = \
  addrdb.h \
  addressindex.h \
  assetdb.h \
  spentindex.h \
  timestampindex.h \
  addressbook.h \
  addrman.h \
  asset.h \
  assetsdir.h \
  attributes.h \
  banman.h \
  base58.h \
  bech32.h \
  blech32.h \
  blind.h \
  blockencodings.h \
  blockfilter.h \
  bloom.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/cpuid.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  confidential_validation.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  dbwrapper.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  index/base.h \
  index/blockfilterindex.h \
  index/disktxpos.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  interfaces/chain.h \
  interfaces/handler.h \
  interfaces/node.h \
  interfaces/wallet.h \
  issuance.h \
  key.h \
  key_io.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  node/coin.h \
  node/coinstats.h \
  node/context.h \
  node/psbt.h \
  node/transaction.h \
  node/ui_interface.h \
  node/utxo_snapshot.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pos.h \
  pow.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/util.h \
  scheduler.h \
  script/compiler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/miniscript.h \
  script/sigcache.h \
  script/sigencoding.h \
  script/sighashtype.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  signet.h \
  streams.h \
  smsg/db.h \
  smsg/crypter.h \
  smsg/net.h \
  smsg/smessage.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadinterrupt.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txrequest.h \
  txmempool.h \
  undo.h \
  util/asmap.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/error.h \
  util/fees.h \
  util/getuniquepath.h \
  util/golombrice.h \
  util/hasher.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/rbf.h \
  util/ref.h \
  util/settings.h \
  util/sock.h \
  util/spanparsing.h \
  util/string.h \
  util/system.h \
  util/threadnames.h \
  util/time.h \
  util/trace.h \
  util/translation.h \
  util/ui_change_type.h \
  util/url.h \
  util/vector.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  versionbitsinfo.h \
  wallet/bdb.h \
  wallet/coincontrol.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/dump.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/rpcwallet.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/sqlite.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  walletinitinterface.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
librain_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between raind and rain-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# librain_common or librain_util libraries, instead.
librain_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
librain_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  assetdb.cpp \
  primitives/assetsdir.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  confidential_validation.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  flatfile.cpp \
  httprpc.cpp \
  httpserver.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/txindex.cpp \
  init.cpp \
  mapport.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/context.cpp \
  node/interfaces.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/ui_interface.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pos.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/contracts.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  signet.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txrequest.cpp \
  txmempool.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(RAIN_CORE_H)

if ENABLE_WALLET
librain_server_a_SOURCES += wallet/init.cpp
endif
if !ENABLE_WALLET
librain_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
librain_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES) $(ZMQ_CFLAGS)
librain_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between raind and rain-qt, but only linked
# when wallet enabled
librain_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES) $(SQLITE_CFLAGS)
librain_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_wallet_a_SOURCES = \
  addressbook.cpp \
  blind.cpp \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/dump.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/interfaces.cpp \
  wallet/load.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(RAIN_CORE_H)

if USE_SQLITE
librain_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
librain_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif

librain_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(RAIN_CORE_H)

# crypto primitives library
crypto_librain_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_librain_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_librain_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha_poly_aead.h \
  crypto/chacha_poly_aead.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/muhash.h \
  crypto/muhash.cpp \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h

if USE_ASM
crypto_librain_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_librain_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_librain_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_librain_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_librain_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_librain_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_librain_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_librain_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_librain_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_librain_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_librain_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_librain_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_librain_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_librain_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
crypto_librain_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
crypto_librain_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
librain_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/asset.cpp \
  primitives/block.cpp \
  primitives/block.h \
  primitives/confidential.cpp \
  primitives/confidential.h \
  primitives/txwitness.cpp \
  primitives/txwitness.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/rainconsensus.cpp \
  script/compiler.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  script/sigencoding.cpp \
  serialize.h \
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  version.h

# common: shared between raind, and rain-qt and non-server tools
librain_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  blech32.cpp \
  bloom.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  issuance.cpp \
  key.cpp \
  smsg/crypter.cpp \
  key_io.cpp \
  merkleblock.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  psbt.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/miniscript.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  versionbitsinfo.cpp \
  warnings.cpp \
  $(RAIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
librain_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/handler.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/settings.cpp \
  util/threadnames.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  $(RAIN_CORE_H)

if USE_LIBEVENT
librain_util_a_SOURCES += util/url.cpp
endif

librain_smsg_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_smsg_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_smsg_a_CFLAGS = $(AM_CFLAGS) $(PIE_FLAGS)
librain_smsg_a_SOURCES = \
  lz4/lz4.h \
  lz4/lz4.c \
  xxhash/xxhash.h \
  xxhash/xxhash.c \
  interfaces/handler.cpp \
  smsg/crypter.cpp \
  smsg/keystore.h \
  smsg/keystore.cpp \
  smsg/db.cpp \
  smsg/smessage.cpp \
  smsg/rpcsmessage.cpp

if GLIBC_BACK_COMPAT
librain_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between rain-cli and rain-qt
librain_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(RAIN_CORE_H)

nodist_librain_util_a_SOURCES = $(srcdir)/obj/build.h
#

# raind & rain-node binaries #
rain_daemon_sources = raind.cpp
rain_bin_cppflags = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
rain_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
rain_daemon_sources += raind-res.rc
endif

rain_bin_ldadd = \
  $(LIBRAIN_SMSG) \
  $(LIBRAIN_SERVER) \
  $(LIBRAIN_WALLET) \
  $(LIBRAIN_COMMON) \
  $(LIBRAIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBRAIN_ZMQ) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

rain_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

raind_SOURCES = $(rain_daemon_sources)
raind_CPPFLAGS = $(rain_bin_cppflags)
raind_CXXFLAGS = $(rain_bin_cxxflags)
raind_LDFLAGS = $(rain_bin_ldflags)
raind_LDADD = $(LIBRAIN_SERVER) $(rain_bin_ldadd)

rain_node_SOURCES = $(rain_daemon_sources)
rain_node_CPPFLAGS = $(rain_bin_cppflags)
rain_node_CXXFLAGS = $(rain_bin_cxxflags)
rain_node_LDFLAGS = $(rain_bin_ldflags)
rain_node_LDADD = $(LIBRAIN_SERVER) $(rain_bin_ldadd)

# rain-cli binary #
rain_cli_SOURCES = rain-cli.cpp
rain_cli_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES) $(EVENT_CFLAGS)
rain_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
rain_cli_SOURCES += rain-cli-res.rc
endif

rain_cli_LDADD = \
  $(LIBRAIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_CRYPTO)

rain_cli_LDADD += $(BOOST_LIBS) $(EVENT_LIBS)
#

# rain-tx binary #
rain_tx_SOURCES = rain-tx.cpp
rain_tx_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
rain_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
rain_tx_SOURCES += rain-tx-res.rc
endif

rain_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBRAIN_COMMON) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_CRYPTO) \
  $(LIBSECP256K1)

rain_tx_LDADD += $(BOOST_LIBS)
#

# rain-wallet binary #
rain_wallet_SOURCES = rain-wallet.cpp
rain_wallet_CPPFLAGS = $(rain_bin_cppflags)
rain_wallet_CXXFLAGS = $(rain_bin_cxxflags)
rain_wallet_LDFLAGS = $(rain_bin_ldflags)
rain_wallet_LDADD = $(LIBRAIN_WALLET_TOOL) $(rain_bin_ldadd)

if TARGET_WINDOWS
rain_wallet_SOURCES += rain-wallet-res.rc
endif
#

# rain-util binary #
rain_util_SOURCES = rain-util.cpp
rain_util_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
rain_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
rain_util_SOURCES += rain-util-res.rc
endif

rain_util_LDADD = \
  $(LIBRAIN_COMMON) \
  $(LIBRAIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_CRYPTO) \
  $(LIBSECP256K1)

rain_util_LDADD += $(BOOST_LIBS)
#

# rainconsensus library #
if BUILD_RAIN_LIBS
include_HEADERS = script/rainconsensus.h
librainconsensus_la_SOURCES = support/cleanse.cpp $(crypto_librain_crypto_base_a_SOURCES) $(librain_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  librainconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

librainconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
librainconsensus_la_LIBADD = $(LIBSECP256K1)
librainconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_RAIN_INTERNAL
librainconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/rain-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/rain-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/rain-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/rain-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if TARGET_DARWIN
	@echo "Checking macOS dynamic libraries..."
	$(AM_V_at) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_WINDOWS
	@echo "Checking Windows dynamic libraries..."
	$(AM_V_at) OBJDUMP=$(OBJDUMP) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) CPPFILT=$(CPPFILT) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) OBJDUMP=$(OBJDUMP) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

if EMBEDDED_LEVELDB
include Makefile.crc32c.include
include Makefile.leveldb.include
endif

include Makefile.test_util.include
include Makefile.test_fuzz.include

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
