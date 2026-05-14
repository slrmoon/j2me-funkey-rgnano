#!/bin/bash
set -e

export ENABLE_COMPILATION_WARNINGS=true
export USE_DEBUG_ROMGEN_SYMBOLS=true

export MEHOME=$(pwd)/phoneme_feature
mkdir -p $MEHOME/build_output 
export BUILD_OUTPUT_DIR=$MEHOME/build_output
export JDK_DIR=~/j2sdk1.4.2_19
export PATH=$JDK_DIR/bin:$PATH

#export MONTAVISTA=/gp2xsdk/Tools/arm-gp2x-linux

export PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl
export PCSL_PLATFORM=linux_i386_gcc

cd $MEHOME/pcsl
make  NETWORK_MODULE=bsd/generic

#export PCSL_PLATFORM=linux_arm_gcc
#cd $MEHOME/pcsl
#make  NETWORK_MODULE=bsd/generic GNU_TOOLS_DIR=$MONTAVISTA

export JVMWorkSpace=$MEHOME/cldc
export JVMBuildSpace=$BUILD_OUTPUT_DIR/cldc
#cd $JVMWorkSpace/build/linux_arm
#make ENABLE_PCSL=true PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl ENABLE_ISOLATES=true GNU_TOOLS_DIR=$MONTAVISTA

cd $JVMWorkSpace/build/linux_i386
export PCSL_PLATFORM=linux_i386_gcc
make ENABLE_PCSL=true PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl ENABLE_ISOLATES=true 

export MIDP_OUTPUT_DIR=$BUILD_OUTPUT_DIR/midp
cd $MEHOME/midp/build/linux_sdl_gcc
make USE_SDL_ABB=true PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/linux_i386/dist TOOLS_DIR=$MEHOME/tools USE_MULTIPLE_ISOLATES=true

#cd $MEHOME/midp/build/linux_sdl_gcc
#make USE_SDL_ABB=true PCSL_OUTPUT_DIR=$BUILD_OUTPUT_DIR/pcsl CLDC_DIST_DIR=$BUILD_OUTPUT_DIR/cldc/linux_arm/dist TOOLS_DIR=$MEHOME/tools TARGET_CPU=arm USE_MULTIPLE_ISOLATES=true 
GNU_TOOLS_DIR=$MONTAVISTA

