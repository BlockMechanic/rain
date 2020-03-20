# Copyright (c) 2013-2016 The Rain Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) -fPIC
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) -fPIC

AM_CFLAGS = -fPIC
AM_LIBTOOLFLAGS = --preserve-dup-deps
EXTRA_LIBRARIES =

if ENABLE_CRASH_HOOKS
if CRASH_HOOKS_WRAPPED_CXX_ABI
# Wrap internal C++ ABI's so that we can attach stacktraces to exceptions
LDFLAGS_WRAP_EXCEPTIONS = -Wl,-wrap,__cxa_allocate_exception -Wl,-wrap,__cxa_free_exception
if TARGET_WINDOWS
LDFLAGS_WRAP_EXCEPTIONS += -Wl,-wrap,_assert -Wl,-wrap,_wassert
else
LDFLAGS_WRAP_EXCEPTIONS += -Wl,-wrap,__assert_fail
endif
endif
endif

if TARGET_WINDOWS
BACKTRACE_LIB = -ldbghelp -lbacktrace
else
BACKTRACE_LIB = -lbacktrace
endif

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

RAIN_INCLUDES=-I$(builddir) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

RAIN_INCLUDES += -I$(srcdir)/secp256k1/include
RAIN_INCLUDES += $(UNIVALUE_CFLAGS)

BLS_LIBS=-lchiabls -lgmp

LIBRAIN_SERVER=librain_server.a
LIBRAIN_COMMON=librain_common.a
LIBRAIN_CONSENSUS=librain_consensus.a
LIBRAIN_CLI=librain_cli.a
LIBRAIN_UTIL=librain_util.a
LIBRAIN_CRYPTO_BASE=crypto/librain_crypto_base.a
LIBRAINQT=qt/librainqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

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
  $(LIBRAIN_ZMQ)

lib_LTLIBRARIES = $(LIBRAINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_RAIND
  bin_PROGRAMS += raind
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

.PHONY: FORCE check-symbols check-security
# rain core #
RAIN_CORE_H = \
  addrdb.h \
  addressindex.h \
  spentindex.h \
  addrman.h \
  attributes.h \
  banman.h \
  base58.h \
  batchedlogger.h \
  bignum.h \
  bech32.h \
  bloom.h \
  blockencodings.h \
  blockfilter.h \
  bls/bls_batchverifier.h \
  bls/bls_ies.h \
  bls/bls_worker.h \
  cachemap.h \
  cachemultimap.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  crosschain/interblockchain.h \
  cuckoocache.h \
  ctpl.h \
  cxxtimer.hpp \
  evo/cbtx.h \
  evo/deterministicmns.h \
  evo/evodb.h \
  evo/mnauth.h \
  evo/providertx.h \
  evo/simplifiedmns.h \
  evo/specialtx.h \
  privatesend/privatesend.h \
  privatesend/privatesend-client.h \
  privatesend/privatesend-server.h \
  privatesend/privatesend-util.h \
  dsnotificationinterface.h \
  governance/governance.h \
  governance/governance-classes.h \
  governance/governance-exceptions.h \
  governance/governance-object.h \
  governance/governance-validators.h \
  governance/governance-vote.h \
  governance/governance-votedb.h \
  flat-database.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  hashblock.h \
  index/base.h \
  index/blockfilterindex.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  interfaces/chain.h \
  interfaces/handler.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  keepass.h \
  key_io.h \
  dbwrapper.h \
  limitedmap.h \
  logging.h \
  llmq/quorums.h \
  llmq/quorums_blockprocessor.h \
  llmq/quorums_commitment.h \
  llmq/quorums_chainlocks.h \
  llmq/quorums_debug.h \
  llmq/quorums_dkgsessionhandler.h \
  llmq/quorums_dkgsessionmgr.h \
  llmq/quorums_dkgsession.h \
  llmq/quorums_init.h \
  llmq/quorums_instantsend.h \
  llmq/quorums_signing.h \
  llmq/quorums_signing_shares.h \
  llmq/quorums_utils.h \
  masternode/activemasternode.h \
  masternode/masternode-meta.h \
  masternode/masternode-payments.h \
  masternode/masternode-sync.h \
  masternode/masternode-utils.h \
  memusage.h \
  merkleblock.h \
  messagesigner.h \
  miner.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netfulfilledman.h \
  netmessagemaker.h \
  node/coin.h \
  node/psbt.h \
  node/transaction.h \
  noui.h \
  obfuscation.h \
  obfuscation-relay.h \
  optional.h \
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
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/util.h \
  saltedhasher.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  smessage/lz4.h \
  smessage/smessage.h \
  smessage/xxhash.h \
  sphlib/sph_types.h \
  sphlib/sph_groestl.h \
  sphlib/sph_shabal.h \
  sphlib/sph_blake.h \
  sphlib/sph_bmw.h \
  sphlib/sph_cubehash.h \
  sphlib/sph_echo.h \
  sphlib/sph_jh.h \
  sphlib/sph_keccak.h \
  sphlib/sph_luffa.h \
  sphlib/sph_shavite.h \
  sphlib/sph_simd.h \
  sphlib/sph_skein.h \
  spork.h \
  stacktraces.h \
  streams.h \
  support/allocators/mt_pooled_secure.h \
  support/allocators/pooled_secure.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  unordered_lru_cache.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/error.h \
  util/fees.h \
  util/system.h \
  util/memory.h \
  util/moneystr.h \
  util/rbf.h \
  util/string.h \
  util/threadnames.h \
  util/time.h \
  util/translation.h \
  util/url.h \
  util/validation.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  versionbitsinfo.h \
  walletinitinterface.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/psbtwallet.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  wallet/coinselection.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h

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
  banman.cpp \
  batchedlogger.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  bls/bls_ies.cpp \
  bls/bls_worker.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/tx_verify.cpp \
  dsnotificationinterface.cpp \
  evo/cbtx.cpp \
  evo/deterministicmns.cpp \
  evo/evodb.cpp \
  evo/mnauth.cpp \
  evo/providertx.cpp \
  evo/simplifiedmns.cpp \
  evo/specialtx.cpp \
  flatfile.cpp \
  httprpc.cpp \
  httpserver.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/txindex.cpp \
  interfaces/chain.cpp \
  interfaces/node.cpp \
  init.cpp \
  dbwrapper.cpp \
  governance/governance.cpp \
  governance/governance-classes.cpp \
  governance/governance-object.cpp \
  governance/governance-validators.cpp \
  governance/governance-vote.cpp \
  governance/governance-votedb.cpp \
  llmq/quorums.cpp \
  llmq/quorums_blockprocessor.cpp \
  llmq/quorums_commitment.cpp \
  llmq/quorums_chainlocks.cpp \
  llmq/quorums_debug.cpp \
  llmq/quorums_dkgsessionhandler.cpp \
  llmq/quorums_dkgsessionmgr.cpp \
  llmq/quorums_dkgsession.cpp \
  llmq/quorums_init.cpp \
  llmq/quorums_instantsend.cpp \
  llmq/quorums_signing.cpp \
  llmq/quorums_signing_shares.cpp \
  llmq/quorums_utils.cpp \
  masternode/activemasternode.cpp \
  masternode/masternode-meta.cpp \
  masternode/masternode-payments.cpp \
  masternode/masternode-sync.cpp \
  masternode/masternode-utils.cpp \
  messagesigner.cpp \
  miner.cpp \
  net.cpp \
  netfulfilledman.cpp \
  net_processing.cpp \
  node/coin.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pos.cpp \
  pow.cpp \
  privatesend/privatesend.cpp \
  privatesend/privatesend-server.cpp \
  privatesend/privatesend-client.cpp \
  privatesend/privatesend-util.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/masternode.cpp \
  rpc/governance.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/rpcevo.cpp \
  rpc/rpcquorums.cpp \
  rpc/server.cpp \
  rpc/privatesend.cpp \
  rpc/smessage.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  smessage/lz4.c \
  smessage/smessage.cpp \
  smessage/xxhash.c \
  spork.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
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
  zmq/zmqrpc.cpp
endif


# wallet: shared between raind and rain-qt, but only linked
# when wallet enabled
librain_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_wallet_a_SOURCES = \
  keepass.cpp \
  interfaces/wallet.cpp \
  wallet/coincontrol.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/ismine.cpp \
  wallet/load.cpp \
  wallet/psbtwallet.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(RAIN_CORE_H)

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
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
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
  bls/bls.cpp \
  bls/bls.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  sphlib/sph_shabal.cpp \
  sphlib/skein.c \
  sphlib/blake.c \
  sphlib/bmw.c \
  sphlib/cubehash.c \
  sphlib/echo.c \
  sphlib/luffa.c \
  sphlib/shavite.c \
  sphlib/jh.c \
  sphlib/simd.c \
  sphlib/keccak.c \
  sphlib/groestl.c \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/rainconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
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
  bloom.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  crosschain/interblockchain.cpp \
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
  saltedhasher.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  spork.cpp \
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
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/system.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/threadnames.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  util/url.cpp \
  util/validation.cpp \
  $(RAIN_CORE_H)

if GLIBC_BACK_COMPAT
librain_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between rain-cli and rain-qt
librain_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
librain_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
librain_cli_a_SOURCES = \
  rpc/client.cpp \
  $(RAIN_CORE_H)

nodist_librain_util_a_SOURCES = $(srcdir)/obj/build.h
#

# raind binary #
raind_SOURCES = raind.cpp
raind_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
raind_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
raind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
raind_SOURCES += raind-res.rc
endif

# Libraries below may be listed more than once to resolve circular dependencies (see
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking#circular-dependency)
raind_LDADD = \
  $(LIBRAIN_SERVER) \
  $(LIBRAIN_WALLET) \
  $(LIBRAIN_SERVER) \
  $(LIBRAIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_ZMQ) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

raind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(BLS_LIBS)

# rain-cli binary #
rain_cli_SOURCES = rain-cli.cpp
rain_cli_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES) $(EVENT_CFLAGS)
rain_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
rain_cli_SOURCES += rain-cli-res.rc
endif

rain_cli_LDADD = \
  $(LIBRAIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_CRYPTO)
rain_cli_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS) $(BLS_LIBS)
#

# rain-tx binary #
rain_tx_SOURCES = rain-tx.cpp
rain_tx_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
rain_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

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

rain_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS) $(BLS_LIBS)
#

# rain-wallet binary #
rain_wallet_SOURCES = rain-wallet.cpp
rain_wallet_CPPFLAGS = $(AM_CPPFLAGS) $(RAIN_INCLUDES)
rain_wallet_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
rain_wallet_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
rain_wallet_SOURCES += rain-wallet-res.rc
endif

rain_wallet_LDADD = \
  $(LIBRAIN_WALLET_TOOL) \
  $(LIBRAIN_WALLET) \
  $(LIBRAIN_COMMON) \
  $(LIBRAIN_CONSENSUS) \
  $(LIBRAIN_UTIL) \
  $(LIBRAIN_CRYPTO) \
  $(LIBRAIN_ZMQ) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBUNIVALUE)

rain_wallet_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(CRYPTO_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(MINIUPNPC_LIBS) $(ZMQ_LIBS) $(BLS_LIBS)
#

# rainconsensus library #
if BUILD_RAIN_LIBS
include_HEADERS = script/rainconsensus.h
librainconsensus_la_SOURCES = support/cleanse.cpp $(crypto_librain_crypto_base_a_SOURCES) $(librain_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  librainconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

librainconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
librainconsensus_la_LIBADD = $(LIBSECP256K1) $(BLS_LIBS)
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
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
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
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

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
