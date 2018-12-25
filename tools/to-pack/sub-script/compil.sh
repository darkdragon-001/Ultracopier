#!/bin/bash

function compil {
    TARGET=$1
    DEBUG=$2
    DEBUG_REAL=$3
    PORTABLE=$4
    PORTABLEAPPS=$5
    BITS=$6
    CFLAGSCUSTOM="$7"
    ULTIMATE=$8
    FORPLUGIN=$9
    STATIC=${10}
    CGMINER=${11}
    SUPERCOPIER=${12}
    ULTRACOPIER_VERSION_FINAL=${ULTRACOPIER_VERSION}
    cd ${BASE_PWD}
    rsync -artq --delete ${ULTRACOPIERSOURCESPATH}/ ${TEMP_PATH}/${TARGET}/
    if [ $? -ne 0 ]
    then
        echo rsync -avrt ${ULTRACOPIERSOURCESPATH}/ ${TEMP_PATH}/${TARGET}/ fail into `pwd` $LINENO
        exit 1
    fi
    echo "${TARGET} rsync..."
    for project in `find ${TEMP_PATH}/${TARGET}/plugins/Languages/ -mindepth 1 -type d`
    do
        cd ${project}/
        if [ -f *.ts ]
        then
            lrelease -nounfinished -compress -removeidentical *.ts > /dev/null 2>&1
        fi
        cd ${TEMP_PATH}/${TARGET}/
    done
    if [ $SUPERCOPIER -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i "s/=ultracopier/=supercopier/g" {} \;
        find ${TEMP_PATH}/${TARGET}/ -name "resources-windows.rc" -exec sed -i "s/Ultracopier/Supercopier/g" {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}/ -name "resources-windows.rc" -exec sed -i "s/ultracopier.exe/supercopier.exe/g" {} \; > /dev/null 2>&1
        mv ${TEMP_PATH}/${TARGET}/resources/supercopier-16x16.png ${TEMP_PATH}/${TARGET}/resources/ultracopier-16x16.png
        mv ${TEMP_PATH}/${TARGET}/resources/supercopier-128x128.png ${TEMP_PATH}/${TARGET}/resources/ultracopier-128x128.png
        mv ${TEMP_PATH}/${TARGET}/resources/supercopier-all-in-one.ico ${TEMP_PATH}/${TARGET}/resources/ultracopier-all-in-one.ico
        mv ${TEMP_PATH}/${TARGET}/resources/supercopier.ico ${TEMP_PATH}/${TARGET}/resources/ultracopier.ico
        mv ${TEMP_PATH}/${TARGET}/resources/supercopier.icns ${TEMP_PATH}/${TARGET}/resources/ultracopier.icns
        #		rm -Rf ${TEMP_PATH}/${TARGET}/plugins/Themes/Oxygen/
    fi
    find ${TEMP_PATH}/${TARGET}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION_FINAL}<\/version>/g" {} \; > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<pubDate>.*<\pubDate>/<pubDate>`date +%s`<\pubDate>/g" {} \; > /dev/null 2>&1
    if [ $DEBUG -eq 1 ]
    then
        echo 'CONFIG   += console' >> ${TEMP_PATH}/${TARGET}/other-pro/ultracopier-core.pro
        echo '' >> ${TEMP_PATH}/${TARGET}/other-pro/ultracopier-core.pro
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
    fi
    if [ $SUPERCOPIER -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_MODE_SUPERCOPIER/#define ULTRACOPIER_MODE_SUPERCOPIER/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_MODE_SUPERCOPIER/\/\/#define ULTRACOPIER_MODE_SUPERCOPIER/g" {} \; > /dev/null 2>&1
    fi
    if [ $STATIC -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
    fi
    if [ $ULTIMATE -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_ULTIMATE/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
    fi
    if [ $PORTABLE -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_PORTABLE/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
    fi
    if [ $PORTABLEAPPS -eq 1 ]
    then
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
    else
        find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
    fi
    if [ ${BITS} -eq 32 ]
    then
        MXEPATH='/home/mxe-i686-w64-mingw32-shared-qt5/'
        MXEPATHQMAKE='/home/mxe-i686-w64-mingw32-shared-qt5/usr/bin/i686-w64-mingw32.shared-qmake-qt5'
        export PATH=/home/mxe-i686-w64-mingw32-shared-qt5/usr/bin:$PATH
    fi
    if [ ${BITS} -eq 64 ]
    then
        MXEPATH='/home/mxe-x86_64-w64-mingw32-shared-qt5/'
        MXEPATHQMAKE='/home/mxe-x86_64-w64-mingw32-shared-qt5/usr/bin/x86_64-w64-mingw32.shared-qmake-qt5'
        export PATH=/home/mxe-x86_64-w64-mingw32-shared-qt5/usr/bin:$PATH
    fi
    if [ ${STATIC} -eq 1 ]
    then
        if [ ${BITS} -eq 32 ]
        then
            find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86<\/architecture>/g" {} \; > /dev/null 2>&1
            #			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-32Bits-static-for-ultracopier/"
        fi
        if [ ${BITS} -eq 64 ]
        then
            find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
            #	                REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-64Bits-static-for-ultracopier/"
        fi
    else
        if [ ${BITS} -eq 32 ]
        then
            find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86<\/architecture>/g" {} \; > /dev/null 2>&1
            #			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-32Bits-for-ultracopier/"
        fi
        if [ ${BITS} -eq 64 ]
        then
            find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
            #			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-64Bits-for-ultracopier/"
        fi
    fi
    REAL_WINEPREFIX="${MXEPATH}"
    mkdir -p ${REAL_WINEPREFIX}/drive_c/temp/
    if [ ${DEBUG_REAL} -eq 1 ]
    then
        COMPIL_SUFFIX="debug"
        COMPIL_FOLDER="debug"
    else
        COMPIL_SUFFIX="release"
        COMPIL_FOLDER="release"
    fi
    rsync -art --delete ${TEMP_PATH}/${TARGET}/ ${REAL_WINEPREFIX}/drive_c/temp/
    if [ $? -ne 0 ]
    then
        echo line: $LINENO
        echo rsync -art --delete ${TEMP_PATH}/${TARGET}/ ${REAL_WINEPREFIX}/drive_c/temp/
        exit 1
    fi
    cd ${REAL_WINEPREFIX}/drive_c/temp/
    PLUGIN_FOLDER="${REAL_WINEPREFIX}/drive_c/temp/plugins/"
    cd ${PLUGIN_FOLDER}
    for plugins_cat in `ls -1`
    do
        if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
        then
            cd ${PLUGIN_FOLDER}/${plugins_cat}/
            for plugins_name in `ls -1`
            do
                if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ] && [ ! -f ${plugins_name}/*.dll ] && [ ! -f ${plugins_name}/*.a ] && [ "${plugins_name}" != "KDE4" ] && [ "${plugins_name}" != "dbus" ] && [ "${plugins_name}" != "keybinding" ] && [ "${plugins_name}" != "ultracopier-keybinding" ]
                then
                    #					echo "${TARGET} compilation of the plugin: ${plugins_cat}/${plugins_name}..."
                    cd ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/

                    if [ ${STATIC} -ne 1 ] && [[ (${FORPLUGIN} -eq 1 || "${plugins_name}" != "Rsync") ]]
                    then
                        if [[ ( $SUPERCOPIER -eq 1 && "${plugins_name}" = "Supercopier" ) || ( $SUPERCOPIER -eq 0 && "${plugins_name}" = "Oxygen" ) ]]
                        then
                            echo "${TARGET} compilation of the plugin: ${plugins_cat}/${plugins_name}..."

                            cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                            if [ $? -ne 0 ]
                            then
                                echo error at cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/ $LINENO
                                exit
                            fi
                            if [ ${STATIC} -ne 1 ] && [[ "${plugins_name}" != "Supercopier" ]]
                            then
                                cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/Oxygen/
                                if [ $? -ne 0 ]
                                then
                                    echo error at cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/Oxygen/ $LINENO
                                    exit
                                fi
                            fi
                            echo '' >> *.pro
                            echo 'win32:RC_FILE += resources-windows-ultracopier-plugins.rc' >> *.pro
                            cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ./
                            if [ $? -ne 0 ]
                            then
                                echo error at cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ./ $LINENO
                                exit
                            fi
                            # replace ULTRACOPIER_PLUGIN_VERSION
                            ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
                            sed -i "s/ULTRACOPIER_PLUGIN_VERSION/${ULTRACOPIER_PLUGIN_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                            # replace ULTRACOPIER_PLUGIN_WINDOWS_VERSION
                            ULTRACOPIER_PLUGIN_WINDOWS_VERSION=`echo ${ULTRACOPIER_PLUGIN_VERSION} | sed "s/\./,/g"`
                            sed -i "s/ULTRACOPIER_PLUGIN_WINDOWS_VERSION/${ULTRACOPIER_PLUGIN_WINDOWS_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                            # replace ULTRACOPIER_PLUGIN_NAME
                            sed -i "s/ULTRACOPIER_PLUGIN_NAME/${plugins_name}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                            # replace ULTRACOPIER_PLUGIN_FILENAME
                            ULTRACOPIER_PLUGIN_FILENAME=`grep -F "qtLibraryTarget" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/*.pro | sed -r "s/^.*\((.*)\).*$/\1/g"`
                            sed -i "s/ULTRACOPIER_PLUGIN_FILENAME/${ULTRACOPIER_PLUGIN_FILENAME}.dll/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
                            if [ $? -ne 0 ]
                            then
                                echo ${MXEPATHQMAKE} fail into `pwd` $LINENO
                                exit 1
                            fi
                            if [ ! -f Makefile ]
                            then
                                ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
                                pwd
                                ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
                                echo "plugins not created (makefile not found)"
                                exit
                            fi
                            make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1

                            if [ ! -f ${COMPIL_FOLDER}/*.dll ] && [ ! -f ${COMPIL_FOLDER}/*.a ]
                            then
                                make -j4 ${COMPIL_SUFFIX}
                                pwd
                                echo make -j4 ${COMPIL_SUFFIX}
                                echo "plugins not created (1)"
                                rm -f informations.xml
                                #                        rm -Rf ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                                exit
                            fi
                            if [ ${STATIC} -eq 1 ]
                            then
                                if [ "${COMPIL_FOLDER}" != "./" ]
                                then
                                    cp ${COMPIL_FOLDER}/*.a ./
                                fi
                            else
                                if [ "${COMPIL_FOLDER}" != "./" ]
                                then
                                    mv ${COMPIL_FOLDER}/*.dll ./
                                fi
                            fi
                            #					if [ $STATIC -ne 1 ]
                            #					then
                            #						/usr/bin/find ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/ -type f -name "*.png" -exec rm -f {} \;
                            #					fi
                        fi
                    fi
                else
                    rm -Rf ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                fi
                cd ${PLUGIN_FOLDER}/${plugins_cat}/
            done
            cd ${PLUGIN_FOLDER}/
        fi
    done
    
    
    if [ $ULTIMATE -eq 1 ] || [ $FORPLUGIN -eq 1 ] || [ $SUPERCOPIER -eq 1 ]
    then
        if [ 2 -gt 3 ]
        then
            PLUGIN_FOLDER="${REAL_WINEPREFIX}/drive_c/temp/plugins-alternative/"
            cd ${PLUGIN_FOLDER}/
            for plugins_cat in `ls -1`
            do
                if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
                then
                    cd ${PLUGIN_FOLDER}/${plugins_cat}/
                    for plugins_name in `ls -1`
                    do
                        if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ] && [ ! -f ${plugins_name}/*.dll ] && [ ! -f ${plugins_name}/*.a ] && [ -f ${plugins_name}/informations.xml ] && [ "${plugins_name}" != "KDE4" ] && [ "${plugins_name}" != "dbus" ] && [ "${plugins_name}" != "keybinding" ] && [ "${plugins_name}" != "ultracopier-keybinding" ]
                        then
                            echo "${TARGET} compilation of the plugin: ${plugins_cat}/${plugins_name}..."
                            cd ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/

                            if [ ${STATIC} -ne 1 ]
                            then
                                cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                                echo '' >> *.pro
                                echo 'win32:RC_FILE += resources-windows-ultracopier-plugins.rc' >> *.pro
                                # replace ULTRACOPIER_PLUGIN_VERSION
                                ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
                                sed -i "s/ULTRACOPIER_PLUGIN_VERSION/${ULTRACOPIER_PLUGIN_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                                # replace ULTRACOPIER_PLUGIN_WINDOWS_VERSION
                                ULTRACOPIER_PLUGIN_WINDOWS_VERSION=`echo ${ULTRACOPIER_PLUGIN_VERSION} | sed "s/\./,/g"`
                                sed -i "s/ULTRACOPIER_PLUGIN_WINDOWS_VERSION/${ULTRACOPIER_PLUGIN_WINDOWS_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                                # replace ULTRACOPIER_PLUGIN_NAME
                                sed -i "s/ULTRACOPIER_PLUGIN_NAME/${plugins_name}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                                # replace ULTRACOPIER_PLUGIN_FILENAME
                                ULTRACOPIER_PLUGIN_FILENAME=`grep -F "qtLibraryTarget" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/*.pro | sed -r "s/^.*\((.*)\).*$/\1/g"`
                                sed -i "s/ULTRACOPIER_PLUGIN_FILENAME/${ULTRACOPIER_PLUGIN_FILENAME}.dll/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
                            fi
                            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
                            if [ $? -ne 0 ]
                            then
                                echo ${MXEPATHQMAKE} fail into `pwd` $LINENO
                                exit 1
                            fi
                            make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1
                            if [ ! -f ${COMPIL_FOLDER}/*.dll ] && [ ! -f ${COMPIL_FOLDER}/*.a ]
                            then
                                make -j4 ${COMPIL_SUFFIX}
                                echo "plugins not created: ${plugins_cat}/${plugins_name}"
                                rm -f informations.xml
                                rm -Rf ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                            else
                                if [ ${STATIC} -eq 1 ]
                                then
                                    if [ "${COMPIL_FOLDER}" != "./" ]
                                    then
                                        cp ${COMPIL_FOLDER}/*.a ./
                                    fi
                                else
                                    if [ "${COMPIL_FOLDER}" != "./" ]
                                    then
                                        mv ${COMPIL_FOLDER}/*.dll ./
                                    fi
                                fi
                            fi
                            if [ $STATIC -ne 1 ]
                            then
                                /usr/bin/find ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/ -type f -name "*.png" -exec rm -f {} \;
                            fi
                        else
                            rm -Rf ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
                        fi
                        cd ${PLUGIN_FOLDER}/${plugins_cat}/
                    done
                    cd ${PLUGIN_FOLDER}/
                fi
            done
        fi
    fi
    
    
    
    if [ ${STATIC} -eq 1 ]
    then
        cp ${TEMP_PATH}/${TARGET}/plugins/*/*/*/*.a ${TEMP_PATH}/${TARGET}/plugins/ > /dev/null 2>&1
        cp ${TEMP_PATH}/${TARGET}/plugins-alternative/Themes/Supercopier/*/*.a ${TEMP_PATH}/${TARGET}/plugins/ > /dev/null 2>&1
    fi
    cd ${REAL_WINEPREFIX}/drive_c/temp/
    if [ ${STATIC} -eq 1 ]
    then
        if [ ${SUPERCOPIER} -eq 1 ]
        then
            echo "${TARGET} supercopier static application..."
            cd other-pro/
            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" supercopier-static.pro
        else
            echo "${TARGET} ultracopier static application..."
            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-static.pro
        fi
    else
        cd other-pro/
        echo "${TARGET} application..."
        if [ ${SUPERCOPIER} -eq 1 ]
        then
            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" supercopier-core.pro
        else
            ${MXEPATHQMAKE} QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-core.pro
        fi
    fi
    if [ $? -ne 0 ]
    then
        echo ${MXEPATHQMAKE} fail into `pwd` $LINENO
        exit 1
    fi
    make -j4 ${COMPIL_SUFFIX} > /dev/null 2>&1
    if [ ! -f ${COMPIL_FOLDER}/ultracopier.exe ]
    then
        make -j4 ${COMPIL_SUFFIX} > /tmp/bug.log 2>&1
        if [ ! -f ${COMPIL_FOLDER}/ultracopier.exe ]
        then
            cat /tmp/bug.log
            echo "application not created"
            exit
        fi
    fi
    cd ${REAL_WINEPREFIX}/drive_c/temp/
    if [ $SUPERCOPIER -eq 1 ]
    then
        rm -Rf ${TEMP_PATH}/${TARGET}/plugins/Themes/Oxygen/
    fi
    rsync -art --delete ${REAL_WINEPREFIX}/drive_c/temp/ ${TEMP_PATH}/${TARGET}/
    if [ $? -ne 0 ]
    then
        echo line: $LINENO
        echo rsync -art --delete ${REAL_WINEPREFIX}/drive_c/temp/ ${TEMP_PATH}/${TARGET}/
        exit 1
    fi
    rm -Rf ${REAL_WINEPREFIX}/drive_c/temp/
    cd ${TEMP_PATH}/${TARGET}/
    if [ "${COMPIL_FOLDER}" != "./" ]
    then
        if [ ! -e other-pro/${COMPIL_FOLDER}/ultracopier.exe ]
        then
            echo ${COMPIL_FOLDER}/ultracopier.exe not found into `pwd`
            exit
        fi
        mv other-pro/${COMPIL_FOLDER}/ultracopier.exe ./
    fi
    if [ 1 == 2 ]
    then
        if [ ${BITS} -eq 32 ] && [ ${DEBUG_REAL} -ne 1 ]
        then
            upx --lzma -9 ultracopier.exe > /dev/null 2>&1
        fi
    fi
    if [ $SUPERCOPIER -eq 1 ]
    then
        if [ ! -e ultracopier.exe ]
        then
            echo ultracopier.exe not found into `pwd`
            exit
        fi
        mv ultracopier.exe supercopier.exe
    fi
    if [ $ULTIMATE -ne 1 ] && [ $FORPLUGIN -ne 1 ] && [ $SUPERCOPIER -ne 1 ]
    then
        rm -Rf ${TEMP_PATH}/${TARGET}/plugins-alternative/
    fi
    /usr/bin/find ${TEMP_PATH}/${TARGET}/ -type f -not \( -name "*.xml" -or -name "*.dll" -or -name "*.a" -or -name "*.exe" -or -name "*.txt" -or -name "*.qm" \) -exec rm -f {} \;
    rm -Rf ${TEMP_PATH}/${TARGET}/resources/ ${PLUGIN_FOLDER}/SessionLoader/KDE4/
    rm -Rf ${TEMP_PATH}/${TARGET}/resources/ ${PLUGIN_FOLDER}/Listener/dbus/
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
    echo "${TARGET} compilation... done"
}
