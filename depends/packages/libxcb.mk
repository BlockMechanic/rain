package=libxcb
$(package)_version=1.11.1
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=b720fd6c7d200e5371affdb3f049cc8f88cff9aed942ff1b824d95eedbf69d30
$(package)_dependencies=xcb_proto libXau libxkbcommon

define $(package)_set_vars
$(package)_config_opts=--disable-static --disable-devel-docs --without-doxygen --without-launchd
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
# Disable uneeded extensions.
# More info is available from: https://doc.qt.io/qt-5.15/linux-requirements.html
$(package)_config_opts += --disable-composite --disable-damage --disable-dpms
$(package)_config_opts += --disable-dri2 --disable-dri3 --disable-glx
$(package)_config_opts += --disable-present --disable-record --disable-resource
$(package)_config_opts += --disable-screensaver --disable-xevie --disable-xfree86-dri
$(package)_config_opts += --disable-xinput --disable-xprint --disable-selinux
$(package)_config_opts += --disable-xtest --disable-xv --disable-xvmc
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux &&\
  sed "s/pthread-stubs//" -i configure
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share/man share/doc lib/*.la
endef
