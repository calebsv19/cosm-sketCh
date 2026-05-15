PROGRAM_KEY := drawing_program
RELEASE_PRODUCT_NAME := sketCh
RELEASE_BUNDLE_ID := com.cosm.sketch
LAUNCHER_BIN := sketch-launcher
APP_BIN := drawing-program-bin
VERSION_FILE := VERSION
RELEASE_CHANNEL ?= stable

HOST_CC ?= cc
FISICS_CC ?= /Users/calebsv/Desktop/CodeWork/fisiCs/fisics
BUILD_TOOLCHAIN ?= clang
PACKAGE_TOOLCHAIN ?= $(BUILD_TOOLCHAIN)
TEST_TOOLCHAIN := clang
PKG_CONFIG ?= pkg-config
