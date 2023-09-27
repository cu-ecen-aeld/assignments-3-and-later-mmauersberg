#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

# TASKS (that were TODOs) by Martin Mauersberg

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# Patch that needs to be applied to fix dtc-lexer error
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
PATCH=${SCRIPT_DIR}/dtc-lexer-patch.patch

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TASK: Add your kernel build steps here

    # apply patch
    echo "Applying ${PATCH}..."
    git apply ${PATCH}

    # clean config
    echo "cleaning configuration"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # configure for default config
    echo "configuring defconfig"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # build the kernel
    echo "building linux kernel"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # build kernel modules
    echo "building linux kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # build devicetree
    echo "building devicetree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TASK: Create necessary base directories
echo "Creating base directories"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TASK:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TASK: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

# TASK: Add library dependencies to rootfs
ROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
cp ${ROOT}/lib/ld-linux-aarch64.so.* ${OUTDIR}/rootfs/lib
cp ${ROOT}/lib64/libm.so.* ${OUTDIR}/rootfs/lib64
cp ${ROOT}/lib64/libresolv.so.* ${OUTDIR}/rootfs/lib64
cp ${ROOT}/lib64/libc.so.* ${OUTDIR}/rootfs/lib64

# TASK: Make device nodes
cd "${OUTDIR}/rootfs"
echo "Making device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TASK: Clean and build the writer utility
echo "building the writer utility"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TASK: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
mv writer ${OUTDIR}/rootfs/home/
cp finder.sh ${OUTDIR}/rootfs/home/
cp -r conf/ ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/

# TASK: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TASK: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio