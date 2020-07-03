#!/bin/bash
#
# Copyright (C) 2018 Pinecone
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

boards=()
commands=()

ROOTDIR=$(cd "$( dirname "$0"  )" && pwd)
OUTDIR=${ROOTDIR}/out
BUILDDIR=${ROOTDIR}/build
MKBOOTDIR=${ROOTDIR}/vendor/tools/host/mkbootimg
ROOTDIR=${ROOTDIR}/nuttx
SIGNKEY=none

PROJECTS=(\
		"abies" \
		"banks" \
		"u1" \
		"u11" \
		"u2" \
		"u3" \
		"v1" )

CONFIGS=(\
		"
		abies/audio
		" \
		"
		banks/a7
		banks/audio
		banks/rpm
		banks/sensor
		" \
		"
		u1/ap
		u1/aplite
		u1/ck
		u1/cp
		u1/sp
		u1/recovery
		" \
		"
		u11/ap
		u11/ck
		u11/cp
		" \
		"
		u2/ap
		u2/audio
		u2/ck
		u2/m4
		" \
		"
		u3/ap
		u3/cpr
		u3/cpx
		u3/qemu
		" \
		"
		v1/vision
		v1/isp
		v1/rpm
		" \
		)

DEFAULT_PROJECT=${PROJECTS[2]}

SYSTEM=`uname -s`
HOST_FLAG=

if [[ "${SYSTEM}" =~ "Linux" ]]; then
	HOST_FLAG=-l
elif [[ "${SYSTEM}" =~ "CYGWIN" ]]; then
	HOST_FLAG=-c
else
	echo "Unsupport OS : ${SYSTEM}"
	exit 1
fi

function get_config_list()
{
	for (( i = 0; i < ${#PROJECTS[*]}; i++)); do
		if [ $1 == ${PROJECTS[$i]} ]; then
			echo ${CONFIGS[$i]}
			break
		fi
	done
}

function usage()
{
	local i=1 j=1
	echo -e "\n----------------------------------------------------"
	echo -e "\nNuttx build shell: [-o product path] [-p project name] [ \${CONFIGS} ]\n"
	echo -e " ${i}. Projects:" && let i++
	for project in ${PROJECTS[*]}; do
		echo "      ($j) ${project}" && let j++
	done
	echo -e "\n    (Configure target project through the -p option:"
	echo -e "-->	./build.sh -p ${DEFAULT_PROJECT})"

	j=1
	echo -e "\n ${i}. Project Board Configs:" && let i++
	for config in ${CONFIGS[*]}; do
		echo "      ($j) ${config}" && let j++
	done
	echo -e "\n    (Support compile one or more of the configurations through:"
	echo -e "-->	./build.sh `get_config_list ${DEFAULT_PROJECT}`)"

	echo -e "\n ${i}. Default Output Directory:" && let i++
	echo -e "       ${OUTDIR}/\${CONFIGS}"
	echo -e "\n    (Configure this path dynamically through the -o option:"
	echo "-->       ./build.sh -o ${OUTDIR}"
	echo "-->       ./build.sh -o ${OUTDIR} `get_config_list ${DEFAULT_PROJECT}`)"

	j=1
	echo -e "\n ${i}. Compile Products:" && let i++
		for config in ${CONFIGS[*]}; do
			echo "      ($j) ${OUTDIR}/${config}.bin" && let j++
		done
	echo -e "\n----------------------------------------------------"
}

function make_system()
{
	if [ ! `command -v ${1}/tools/genromfs > /dev/null` ]; then
		${1}/tools/genromfs -f $2 -d $3 -V "SYSTEM"
	else
		echo "Error: #### Unable to find command 'genromfs', make system image fail ####"
	fi
}

function build_board()
{
	local core=${1#song/}
	local product=${core}.bin
	local elf=${core}.elf
	local coff=${core}.coff
	local apps_out=${OUTDIR}/${core}/apps
	local product_out=${OUTDIR}/${core}/nuttx

	echo -e "\nCompile Command line:\n"
	echo -e "	${ROOTDIR}/tools/configure.sh ${HOST_FLAG} -o ${product_out} ${1}"
	echo -e "	make -C ${ROOTDIR} O=${product_out} ${commands[*]}"
	echo -e "	make -C ${ROOTDIR} O=${product_out} savedefconfig\n"

	${ROOTDIR}/tools/configure.sh ${HOST_FLAG} -o ${product_out} ${1} && \
	make -C ${ROOTDIR} O=${product_out} ${commands[*]}

	if [ $? -ne 0 ]; then
		echo "Error: ############# build ${1} fail ##############"
		exit $?
	fi

	if [[ "${commands[@]}" =~ "distclean" ]]; then
		return;
	fi

	make -C ${ROOTDIR} O=${product_out} savedefconfig

	if [ $? -ne 0 ]; then
		echo "Error: ############# savedefconfig ${1} fail ##############"
		exit $?
	fi

	if [ -f ${product_out}/defconfig ]; then
		cp ${product_out}/defconfig ${ROOTDIR}/boards/${1}
	fi

	if [ -f ${product_out}/nuttx.bin ]; then
		cp -f ${product_out}/nuttx.bin ${OUTDIR}/${product}
	fi

	if [[ ${core} == "u2/audio" ]] || [[ ${core} == "u3/cpr" ]]; then
		if [ -f ${product_out}/nuttx.strip ]; then
			cp -f ${product_out}/nuttx.strip ${OUTDIR}/${elf}
		fi
	fi

	if [[ ${core} == "u3/cpx" ]]; then
		if [ -f ${product_out}/nuttx.a ]; then
			cp -f ${product_out}/nuttx.a ${OUTDIR}/${coff}
		fi
	fi

	if [ -d ${apps_out}/exe/system ]; then
		make_system ${product_out} ${OUTDIR}/${core}/../system.bin ${apps_out}/exe/system
	fi

	if [[ ${SIGNKEY} != none ]]; then
		echo "============================ Signature begin ============================"
		make -C ${MKBOOTDIR} PROJECT=${core%/*}
		${MKBOOTDIR}/mkboot --kernel ${product_out}/nuttx.bin --privkey ${SIGNKEY} --pubkeyout ${OUTDIR}/${core%/*}/pubkey.bin -o ${OUTDIR}/${product}
		if [ $? -ne 0 ]; then
			echo "Signature to ${product_out}/nuttx.bin failed!!!"
		else
			echo "Signature ${core}: ${OUTDIR}/${product} success!!!"
			echo "Generate pubkey: ${OUTDIR}/${core%/*}/pubkey.bin success!!!"
		fi
	fi
}

while [ ! -z "$1" ]; do
	case $1 in
		-o )
			shift
			OUTDIR=$1
			;;
		-p )
			shift
			config_list=`get_config_list $1`

			if [ "${config_list[*]}" ]; then
				for config in ${config_list[*]}; do
					boards[${#boards[@]}]=song/${config}
				done
			else
				echo "Error: Unable to find the board or configurations from $1"
				exit 2
			fi
			;;
		-s )
			shift
			SIGNKEY=$1
			;;
		-h )
			usage
			exit 0
			;;
		* )

			find_config=
			if [ -d "${ROOTDIR}/boards/song/$1" ]; then
				boards[${#boards[@]}]=song/$1
				find_config=true
			elif [ -d "${ROOTDIR}/boards/$1" ]; then
				boards[${#boards[@]}]=$1
				find_config=true
			fi

			if [ "${find_config}" == "" ];then
				commands[${#commands[@]}]=$1
			fi
			;;
	esac
	shift
done

if [ -n "${boards[*]}" ]; then
	for config in ${boards[*]}; do
		build_board ${config}
	done
	exit $?
else
	boards=`get_config_list ${DEFAULT_PROJECT}`
	for config in ${boards[*]}; do
		build_board song/${config}
	done
fi
