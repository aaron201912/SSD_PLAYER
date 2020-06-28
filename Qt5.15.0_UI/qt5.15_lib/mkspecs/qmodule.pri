EXTRA_INCLUDEPATH += /home/koda.xu/Qt/tslib_1.21/arm_tslib/include
EXTRA_LIBDIR += /home/koda.xu/Qt/tslib_1.21/arm_tslib/lib
host_build {
    QT_CPU_FEATURES.x86_64 = mmx sse sse2
} else {
    QT_CPU_FEATURES.arm = neon
}
QT.global_private.enabled_features = alloca_h alloca dbus dlopen gc_binaries gui network posix_fallocate reduce_exports release_tools sql testlib widgets xml
QT.global_private.disabled_features = android-style-assets sse2 alloca_malloc_h avx2 dbus-linked private_tests libudev reduce_relocations relocatable stack-protector-strong system-zlib zstd
QMAKE_LIBS_LIBDL = -ldl
QT_COORD_TYPE = double
CONFIG += cross_compile compile_examples enable_new_dtags largefile neon precompile_header
QT_BUILD_PARTS += libs
QT_HOST_CFLAGS_DBUS += -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
