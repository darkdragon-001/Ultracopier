#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

COMPIL_DEBUGREAL=0
COMPIL_NORMAL=1
COMPIL_PLUGIN=1
COMPIL_ULTIMATE=1

ARCHITECTURE="x86_64"

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh


if [ ${COMPIL_DEBUGREAL} -eq 1 ]
then
        compil "ultracopier-debug-real-windows-x86_64" 1 1 0 0 64 "-mtune=generic -march=nocona" 0 0 0 0 0
	assemble "ultracopier-debug-real" "${ARCHITECTURE}" 1 1 0 0 0 0 0
fi
if [ ${COMPIL_NORMAL} -eq 1 ]
then
	#compil "supercopier-debug-windows-x86_64" 1 0 0 0 64 "-mtune=generic -march=nocona" 0 0 0 0 1
	#assemble "supercopier-debug" "${ARCHITECTURE}" 1 0 0 0 0 0 1
	#compil "supercopier-portable-windows-x86_64" 0 0 1 0 64 "-mtune=generic -march=nocona" 0 0 0 0 1
	#assemble "supercopier-portable" "${ARCHITECTURE}" 0 0 1 0 0 0 1
	compil "supercopier-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 0 0 0 0 1
	assemble "supercopier" "${ARCHITECTURE}" 0 0 0 0 0 0 1

	#compil "ultracopier-debug-windows-x86_64" 1 0 0 0 64 "-mtune=generic -march=nocona" 0 0 0 0 0
	#assemble "ultracopier-debug" "${ARCHITECTURE}" 1 0 0 0 0 0 0
	#compil "ultracopier-debug-portable-windows-x86_64" 1 0 1 0 64 "-mtune=generic -march=nocona" 0 0 0 0 0
	#assemble "ultracopier-debug-portable" "${ARCHITECTURE}" 1 0 1 0 0 0 0
	#compil "ultracopier-portable-windows-x86_64" 0 0 1 0 64 "-mtune=generic -march=nocona" 0 0 0 0 0
	#assemble "ultracopier-portable" "${ARCHITECTURE}" 0 0 1 0 0 0 0
	compil "ultracopier-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 0 0 0 0 0
	assemble "ultracopier" "${ARCHITECTURE}" 0 0 0 0 0 0 0
	#compil "ultracopier-debug-static-windows-x86_64" 1 0 0 0 64 "-mtune=generic -march=nocona" 0 0 1 0 0
	#assemble "ultracopier-debug-static" "${ARCHITECTURE}" 1 0 0 0 1 0 0
	#compil "ultracopier-debug-portable-static-windows-x86_64" 1 0 1 0 64 "-mtune=generic -march=nocona" 0 0 1 0 0
	#assemble "ultracopier-debug-portable-static" "${ARCHITECTURE}" 1 0 1 0 1 0 0
	#compil "ultracopier-portable-static-windows-x86_64" 0 0 1 0 64 "-mtune=generic -march=nocona" 0 0 1 0 0
	#assemble "ultracopier-portable-static" "${ARCHITECTURE}" 0 0 1 0 1 0 0
	#compil "ultracopier-static-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 0 0 1 0 0
	#assemble "ultracopier-static" "${ARCHITECTURE}" 0 0 0 0 1 0 0
fi

if [ ${COMPIL_ULTIMATE} -eq 1 ]
then
	compil "supercopier-ultimate-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 0 1
	assemble "supercopier-ultimate" "${ARCHITECTURE}" 0 0 0 1 0 0 1
	compil "supercopier-ultimate-cgminer-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 1 1
	assemble "supercopier-ultimate-cgminer" "${ARCHITECTURE}" 0 0 0 1 0 1 1
	#compil "supercopier-ultimate-cgminer-static-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 1 1 1
	#assemble "supercopier-ultimate-cgminer-static" "${ARCHITECTURE}" 0 0 0 1 1 1 1

	compil "ultracopier-ultimate-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 0 0
	assemble "ultracopier-ultimate" "${ARCHITECTURE}" 0 0 0 1 0 0 0
	compil "ultracopier-ultimate-cgminer-windows-x86_64" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 1 0
	assemble "ultracopier-ultimate-cgminer" "${ARCHITECTURE}" 0 0 0 1 0 1 0
	#compil "ultracopier-debug-ultimate-cgminer-windows-x86_64" 1 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 1 0
	#assemble "ultracopier-debug-ultimate-cgminer" "${ARCHITECTURE}" 1 0 0 1 0 1 0
	#compil "ultracopier-ultimate-sse2-windows-x86_64" 0 0 0 0 64 "-msse -msse2" 1 0 0 0 0
	#assemble "ultracopier-ultimate-sse2" "${ARCHITECTURE}" 0 0 0 1 0 0 0
	#compil "ultracopier-ultimate-sse3-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3" 1 0 0 0 0
	#assemble "ultracopier-ultimate-sse3" "${ARCHITECTURE}" 0 0 0 1 0 0 0

    compil "ultracopier-ultimate-core2-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0 0 0 0
    assemble "ultracopier-ultimate-core2" "${ARCHITECTURE}" 0 0 0 1 0 0 0
    compil "ultracopier-ultimate-core-i-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0 0 0 0
    assemble "ultracopier-ultimate-core-i" "${ARCHITECTURE}" 0 0 0 1 0 0 0
    compil "ultracopier-ultimate-k8-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -march=k8" 1 0 0 0 0
    assemble "ultracopier-ultimate-k8" "${ARCHITECTURE}" 0 0 0 1 0 0 0

    compil "supercopier-ultimate-core2-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0 0 0 1
    assemble "supercopier-ultimate-core2" "${ARCHITECTURE}" 0 0 0 1 0 0 1
    compil "supercopier-ultimate-core-i-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0 0 0 1
    assemble "supercopier-ultimate-core-i" "${ARCHITECTURE}" 0 0 0 1 0 0 1
    compil "supercopier-ultimate-k8-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -march=k8" 1 0 0 0 1
    assemble "supercopier-ultimate-k8" "${ARCHITECTURE}" 0 0 0 1 0 0 1

	#compil "ultracopier-ultimate-barcelona-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -march=k8" 1 0 0 0 0
	#assemble "ultracopier-ultimate-barcelona" "${ARCHITECTURE}" 0 0 0 1 0 0 0
	#compil "ultracopier-ultimate-bobcat-windows-x86_64" 0 0 0 0 64 "-march=amdfam10 -mno-3dnow -mcx16 -mpopcnt -mssse3 -mmmx" 1 0 0 0 0
	#assemble "ultracopier-ultimate-bobcat" "${ARCHITECTURE}" 0 0 0 1 0 0 0
	#compil "ultracopier-ultimate-llano-windows-x86_64" 0 0 0 0 64 "-march=amdfam10 -mcx16 -mpopcnt" 1 0 0 0 0
	#assemble "ultracopier-ultimate-llano" "${ARCHITECTURE}" 0 0 0 1 0 0 0
	#compil "ultracopier-ultimate-bulldozer-windows-x86_64" 0 0 0 0 64 "-march=amdfam10 -mcx16 -msahf -maes -mpclmul -mpopcnt -mabm" 1 0 0 0 0
	#assemble "ultracopier-ultimate-bulldozer" "${ARCHITECTURE}" 0 0 0 1 0 0 0
fi

if [ ${COMPIL_PLUGIN} -eq 1 ]
then
	compil "ultracopier-windows-x86_64-for-plugins" 0 0 0 0 64 "-mtune=generic -march=nocona" 0 1 0 0
	#compil "ultracopier-debug-windows-x86_64-for-plugins" 1 0 0 0 64 "-mtune=generic -march=nocona" 0 1 0 0 0

	TARGET="ultracopier"
	find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc ] && [ -f ${plugins_name}/*.dll ]
					then
						echo "pack the ${ARCHITECTURE} windows for the plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
						tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
	done
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc ] && [ -f ${plugins_name}/*.dll ]
					then
						echo "pack the ${ARCHITECTURE} windows for the alternative plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
						tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
	done


	TARGET="ultracopier-debug"
	find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc ] && [ -f ${plugins_name}/*.dll ]
					then
						echo "pack the ${ARCHITECTURE} debug windows for the plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
						tar --c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
	done
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc ] && [ -f ${plugins_name}/*.dll ]
					then
						echo "pack the ${ARCHITECTURE} debug windows for the alternative plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
						tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
	done
fi
