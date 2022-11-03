# Enable distributed compilation if we have a scheduler to use. Note that this adds some required GN args:
# use_debug_fission=false - incompatible with icecc
# clang_use_chrome_plugins=false - incompatible with custom binary
# enable_nacl=false - not needed
# clang_use_default_sample_profile = false - affects release builds, afdo.prof is not available
if [[ "${ICECC_SCHEDULER}" && "${ICECC_SCHEDULER^^}" != "DISABLED" ]]
then
    which icecc nc || (apt update && apt install -qy icecc netcat)
    let NUMJOBS=$(echo -e "listcs\nexit" | nc ${ICECC_SCHEDULER} 8766 | grep x86_64 | cut -d'/' -f2 | cut -d' ' -f 1 | paste -s -d+)
    echo "Enabling distributed build with icecc scheduler ${ICECC_SCHEDULER} with ${NUMJOBS} job slots"
    rm -fr  icecc-tarball
    mkdir icecc-tarball
    cd icecc-tarball
    icecc-create-env --clang `pwd`/../third_party/llvm-build/Release+Asserts/bin/clang
    # Workaround: move some libs around inside the tarball so they are used
    # https://github.com/icecc/icecream/issues/462
    tar xf *.tar.gz
    rm -f *.tar.gz
    mv ./lib/libstdc++.so.6 ./usr/lib/x86_64-linux-gnu/libstdc++.so.6
    tar czf ${RANDOM}.tar.gz *
    # end workaround
    cd -
    export CCACHE_PREFIX=icecc
    export ICECC_VERSION=$(find $(pwd)/icecc-tarball -name "*.tar.gz")
    export ICECC_CLANG_REMOTE_CPP=1
    export GN_EXTRA_ARGS_INTERNAL="use_debug_fission=false clang_use_chrome_plugins=false enable_nacl=false clang_use_default_sample_profile = false"
    pkill iceccd || true
    iceccd --daemonize --no-remote --max-processes 0 --name ${CI_PROJECT_NAME:-`hostname`}-jobid-${CI_JOB_ID:-${USER}} --scheduler-host "${ICECC_SCHEDULER}"
fi