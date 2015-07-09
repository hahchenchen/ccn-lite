# build-test.mk
# Set NO_CORES to the number of logical cores available, if not already set
OS:=$(shell sh -c 'uname -s 2> /dev/null || echo not')
ifeq ($(OS),Linux)
    NO_CORES?=$(shell grep -c ^processor /proc/cpuinfo)
else ifeq ($(OS),Darwin) # Assume OS X
    NO_CORES?=$(shell sysctl -n hw.ncpu)
else
    NO_CORES?=1
endif
export NO_CORES

ifdef VERBOSE
    PRINT_LOG=( echo "Contents of /tmp/$@.log:"; cat "/tmp/$@.log"; echo; exit 1 )
else
    PRINT_LOG=( exit 1 )
endif

BT_RELAY:=bt-relay-nothing \
	bt-relay-barebones \
	bt-relay-vanilla \
	bt-relay-frag \
	bt-relay-authCtrl \
	bt-relay-nfn \
	bt-relay-nack \
	bt-relay-nfn-nack \
	bt-relay-all
BT_LNXKERNEL:=bt-lnxkernel
BT_ALL:=bt-all-vanilla \
	bt-all-nfn
BT_PKT:=bt-pkt-ccnb \
	bt-pkt-ccntlv \
	bt-pkt-cistlv \
	bt-pkt-iottlv \
	bt-pkt-ndntlv
PROFILES:=${BT_RELAY} ${BT_LNXKERNEL} ${BT_ALL} ${BT_PKT}

.PHONY: all clean bt-relay bt-all bt-pkt ${PROFILES}
all: echo-cores ${PROFILES} clean
bt-relay: ${BT_RELAY} clean
bt-all: ${BT_ALL} clean
bt-pkt: ${BT_PKT} clean

echo-cores:
	@bash -c 'printf "\e[3mBuilding using $(NO_CORES) cores:\e[0m\n"'

clean:
	@make clean > /dev/null 2>&1
	@echo ''
	@echo 'See /tmp/bt-*.log for more details.'

bt-relay-nothing:
# Build without any USE_*
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	MODIFIY_FNAME=ccn-lite-relay.c \
	UNSET_VARS="USE_CCNxDIGEST USE_DEBUG USE_DEBUG_MALLOC USE_ECHO \
		USE_ETHERNET USE_HMAC256 USE_HTTP_STATUS USE_IPV4 USE_MGMT \
		USE_NACK USE_NFN USE_NFN_NSTRANS USE_NFN_MONITOR USE_SCHEDULER \
		USE_STATS USE_SUITE_CCNB USE_SUITE_CCNTLV USE_SUITE_CISTLV \
		USE_SUITE_IOTTLV USE_SUITE_NDNTLV USE_SUITE_LOCALRPC USE_UNIXSOCKET" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-barebones:
# Build only with USE_IPV4 and USE_SUITE_NDNTLV
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	MODIFIY_FNAME=ccn-lite-relay.c \
	SET_VARS="USE_IPV4 USE_SUITE_NDNTLV" \
	UNSET_VARS="USE_CCNxDIGEST USE_DEBUG USE_DEBUG_MALLOC USE_ECHO \
		USE_ETHERNET USE_HMAC256 USE_HTTP_STATUS USE_MGMT \
		USE_NACK USE_NFN USE_NFN_NSTRANS USE_NFN_MONITOR USE_SCHEDULER \
		USE_STATS USE_SUITE_CCNB USE_SUITE_CCNTLV USE_SUITE_CISTLV \
		USE_SUITE_IOTTLV USE_SUITE_LOCALRPC USE_UNIXSOCKET" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-vanilla:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-frag:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	MAKE_VARS="USE_FRAG=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-authCtrl:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	MAKE_VARS="USE_SIGNATURES=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-nfn:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-nfn-relay" \
	MAKE_VARS="USE_NFN=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-nack:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay-nack" \
	MAKE_VARS="USE_NFN=1 USE_NACK=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-nfn-nack:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-nfn-relay-nack" \
	MAKE_VARS="USE_NFN=1 USE_NACK=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-relay-all:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-relay" \
	MAKE_VARS="USE_FRAG=1 USE_SIGNATURES=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-lnxkernel:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="ccn-lite-lnxkernel" \
	MAKE_VARS="USE_KRNL=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-all-vanilla:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="all" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-all-nfn:
	@MODE="make" \
	TARGET=$@ \
	MAKE_TARGETS="all" \
	MAKE_VARS="USE_NFN=1" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-pkt-ccnb:
	@MODE="pkt-format" \
	TARGET=$@ \
	PKT_FORMAT="ccnb" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-pkt-ccntlv:
	@MODE="pkt-format" \
	TARGET=$@ \
	PKT_FORMAT="ccntlv" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-pkt-cistlv:
	@MODE="pkt-format" \
	TARGET=$@ \
	PKT_FORMAT="cistlv" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-pkt-iottlv:
	@MODE="pkt-format" \
	TARGET=$@ \
	PKT_FORMAT="iottlv" \
	./build-test-helper.sh || ${PRINT_LOG}

bt-pkt-ndntlv:
	@MODE="pkt-format" \
	TARGET=$@ \
	PKT_FORMAT="ndntlv" \
	./build-test-helper.sh || ${PRINT_LOG}
