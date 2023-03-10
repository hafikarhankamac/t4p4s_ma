# Highlight colours

set -x

cc="\033[1;33m"     # yellow
nn="\033[0m"

APPROX_INSTALL_MB="2500"
FREE_MB="`df --output=avail -m . | tail -1 | tr -d '[:space:]'`"

if [ "$SKIP_CHECK" != "1" ] && [ "$FREE_MB" -lt "$APPROX_INSTALL_MB" ]; then
    echo -e "Bootstrapping requires approximately $cc$APPROX_INSTALL_MB MB$nn of free space"
    echo -e "You seem to have $cc$FREE_MB MB$nn of free space on the current drive"
    echo -e "To force installation, run ${cc}SKIP_CHECK=1 $0$nn"
    exit
else
    echo -e "Installation will use approximately $cc$APPROX_INSTALL_MB MB$nn of space"
fi

MAX_MAKE_JOBS=${MAX_MAKE_JOBS-`nproc --all`}

echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs"
echo Requesting root access...
sudo echo -n ""
echo Root access granted, starting...

if [ "$FRESH" == "1" ]; then
    CLEANUP=1
    unset PROTOBUF_BRANCH
    unset DPDK_VSN
    unset RTE_SDK
    unset RTE_TARGET
    unset P4C
fi

if [ "$CLEANUP" == "1" ]; then
    echo Cleaning previously downloaded files and directories
    sudo rm -rf dpdk*
    sudo rm -rf protobuf
    sudo rm -rf p4c
    sudo rm -rf t4p4s*
    sudo rm -f t4p4s_environment_variables.sh
fi

if [ ! `which curl` ] || [ ! `which git` ]; then
    echo -e "Installing ${cc}curl$nn and ${cc}git$nn"
    sudo apt-get -y install curl git
fi

# Set sensible defaults
export PARALLEL_INSTALL=${PARALLEL_INSTALL-1}
export PROTOBUF_BRANCH=${PROTOBUF_BRANCH-`git ls-remote --refs --tags https://github.com/google/protobuf | tail -1 | cut -f3 -d'/'`}

echo -e "Using ${cc}protobuf$nn branch $cc$PROTOBUF_BRANCH$nn"

ssh-keyscan gitlab.lrz.de >> ~/.ssh/known_hosts

echo Setting software versions...
# defaults are current official versions t4p4s uses
# Note: recent versions of official P4C introduced changes currently incompatible with T4P4S
[ -z "$TAPAS_COMMIT" ] && TAPAS_COMMIT=c29f2cae5fb84cf5696096084594181a61f4c20e
[ -z "$P4C_COMMIT" ] && P4C_COMMIT=2f55fb522058af47eed17182a6a1697e09dc6b85
P4RUNTIME_COMMIT=ef54d874d7bd385b1721a07722c371d02dee245f

echo t4p4s: $TAPAS_COMMIT
echo p4c: $P4C_COMMIT
echo p4runtime: $P4RUNTIME_COMMIT

if [ "$DPDK_VSN" != "" ]; then
    echo -e "Using ${cc}user set DPDK version$nn \$DPDK_VSN=$cc${DPDK_VSN}$nn"
else
    # Get the most recent DPDK version
    vsn=`curl -s "https://fast.dpdk.org/rel/" --list-only \
        | grep ".tar.xz" \
        | sed -e "s/^[^>]*>dpdk-\([0-9.]*\)\.tar\.xz[^0-9]*\([0-9]\{2\}\)-\([a-zA-Z]\{3\}\)-\([0-9]\{4\}\) \([0-9]\{2\}\):\([0-9]\{2\}\).*$/\4 \3 \2 \5 \6 \1/g" \
        | sed -e "s/ \([0-9]\{2\}\)[.]\([0-9]\{2\}\)$/ \1.\2.-1/g" \
        | tr '.' ' ' \
        | sort -k6,6n -k7,7n -k8,8n -k1,1 -k2,2M -k3,3 -k4,4 -k5,5 \
        | tac \
        | cut -d" " -f 6- \
        | sed -e "s/^\([0-9\-]*\) \([0-9\-]*\) \([0-9\-]*\)$/\3 \1.\2/g" \
        | uniq -f1 \
        | head -1`

    vsn=($vsn)

    DPDK_VSN="${vsn[1]}"
    echo -e "Using DPDK version $cc${DPDK_VSN}$nn"
fi

DPDK_FILEVSN="$DPDK_VSN"
#[ "${vsn[0]}" != "-1" ] && DPDK_FILEVSN="$DPDK_VSN.${vsn[0]}"


if [ "$RTE_TARGET" != "" ]; then
    echo -e "Using ${cc}DPDK target$nn RTE_TARGET=$cc$RTE_TARGET$nn"
else
    DPDKCC=gcc
    which clang >/dev/null
    [ $? -eq 0 ] && DPDKCC=clang

    echo -e "DPDK will be compiled using ${cc}$DPDKCC$nn"
    export RTE_TARGET=x86_64-native-linuxapp-$DPDKCC
fi

if [ "$USE_OPTIONAL_PACKAGES" != "" ]; then
    OPT_PACKAGES="python-ipdb python-termcolor python-backtrace python-pip python-yaml python-ujson python-ruamel.yaml"
fi

T4P4S_DIR=t4p4s
[ $# -gt 0 ] && T4P4S_DIR="t4p4s-$1" && T4P4S_CLONE_OPT="$T4P4S_DIR -b $1" && echo -e "Using the $cc$1$nn branch of T4P4S"


echo

# Download libraries
sudo apt-get update && sudo apt-get -y install g++ automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev pkg-config python python-scapy python-ipaddr python-dill python-ujson tcpdump cmake python-setuptools libprotobuf-dev libnuma-dev ccache $OPT_PACKAGES &
WAITPROC_APTGET="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_APTGET"

[ ! -d "dpdk-${DPDK_VSN}" ] && wget -q -o /dev/null http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz &
WAITPROC_DPDK="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_DPDK"

[ ! -d "protobuf" ] && git clone --recursive -b "${PROTOBUF_BRANCH}" https://github.com/google/protobuf &
WAITPROC_PROTOBUF="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_PROTOBUF"

[ ! -d "p4c" ] && git clone https://gitlab+deploy-token-265:wnCdz_rZqesV4iYAGw9b@gitlab.lrz.de/p4/tapas/p4c.git && cd p4c && git checkout $P4C_COMMIT && git submodule update --init --recursive &
WAITPROC_P4C="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_P4C"


# ...
[ ! -d t4p4s ] && git clone https://gitlab+deploy-token-247:-K1yHEhTvygwQsCiJ8tG@gitlab.lrz.de/p4/tapas/t4p4s.git && cd ${T4P4S_DIR} && git checkout $TAPAS_COMMIT && git submodule update --init --recursive && cd .. &
WAITPROC_T4P4S="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_T4P4S"



# Wait for apt-get to finish
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_APTGET"

if [ "$USE_OPTIONAL_PACKAGES" != "" ]; then
    pip install backtrace
fi


# Setup DPDK
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_DPDK"

export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`

cd "$RTE_SDK"
make install DESTDIR="${RTE_TARGET}" T="${RTE_TARGET}" LDFLAGS="-fuse-ld=gold" -j ${MAX_MAKE_JOBS}
cd ..


# Setup protobuf
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_PROTOBUF"

cd protobuf
./autogen.sh
./configure LD=ld.gold
make -j ${MAX_MAKE_JOBS}
sudo make install -j ${MAX_MAKE_JOBS}
sudo ldconfig
cd ..


# Setup p4c
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_P4C"

export P4C=`pwd`/p4c

cd p4c
git checkout $P4C_COMMIT
cd control-plane/p4runtime
git checkout $P4RUNTIME_COMMIT
cd ../..
./bootstrap.sh
cd build
LD=ld.gold cmake ..
make -j ${MAX_MAKE_JOBS}
sudo make install -j ${MAX_MAKE_JOBS}
cd ../..


# Enter t4p4s directory
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_T4P4S"

cat <<EOF >./t4p4s_environment_variables.sh
export DPDK_VSN=${DPDK_VSN}
export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`
export RTE_TARGET=${RTE_TARGET}
export P4C=`pwd`/p4c
export T4P4S=${T4P4S_DIR}
EOF

chmod +x `pwd`/t4p4s_environment_variables.sh
. `pwd`/t4p4s_environment_variables.sh

echo Environment variable config is done
echo -e "Environment variable config is saved in ${cc}`pwd`/t4p4s_environment_variables.sh$nn"

if [[ $(grep "t4p4s_environment_variables.sh" ~/.profile) ]]; then
    echo -e "Your ${cc}~/.profile$nn is ${cc}not modified$nn, as it already calls t4p4s_environment_variables.sh"
else
    echo >> ~/.profile
    echo ". `pwd`/t4p4s_environment_variables.sh" >> ~/.profile
    echo -e "Environment variable config is ${cc}enabled on login$nn: your ${cc}~/.profile$nn will run `pwd`/t4p4s_environment_variables.sh"
fi

cd ${T4P4S_DIR}

# set hlir version
[ -n "$HLIR_COMMIT" ] && cd src/hlir16 && git checkout $HLIR_COMMIT && cd ../..
