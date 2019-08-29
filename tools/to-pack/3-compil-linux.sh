#!/usr/bin/env bash
#
# NOTE Currently this script *must* be run from the project root folder!
#
# Usage: 3-compile-linux.sh BUILD_PATH DEBUG
# Example: 3-compile-linux.sh "./build" "./package" 0

# TODO portable, bits, custom flags (qmake, make), static
# TODO change Variables.h -> compile definitions (maintain plugin specific settings!)
#      -> prefix plugin definitions with plugin name
# TODO allow single plugin compilation
# TODO add option for architecture -> merge with windows/mac
# TODO add option to control log and console output
#      (tee, redirect /dev/std{out,err} /dev/null)
# TODO maintain plugins-unmaintained
# TODO allow project directory different from "."
function compile {
  # set options
  # TODO check number of arguments, otherwise print help text
  BUILD_DIR=${1}
  PACKAGE_DIR=${2}
  DEBUG=${3}
  LOGPATH="/tmp"
  LOGFILE="${LOGPATH}/stdout.log"   # /dev/stdout
  LOGERROR="${LOGPATH}/stderr.log"  # /dev/stderr
  MAKE_FLAGS="-j8"

  # build arguments
  args=" DEFINES+=\"Q_OS_LINUX\""
  if [ -z $BUILD_DIR ]; then
    BUILD_DIR='.'
  fi
  if [ $DEBUG -eq 1 ]; then
    args+=" CONFIG+=debug"
  else
    args+=" CONFIG+=release"
  fi

  # do the work
  # output handling: https://stackoverflow.com/a/692407/3779655
  # TODO preserve own debug information -> return from functions? BUT real-time
  # TODO color preservation when using tee: https://superuser.com/a/751809/260055
  # -> how to use it when calling a function?
  # if command -v unbuffer; then  # check if unbuffer command available
  #_compile "${BUILD_DIR}" "${args}" "${MAKE_FLAGS}" > >(tee -a ${LOGFILE}) 2> >(tee -a ${LOGERROR} >&2)
  _compile "${BUILD_DIR}" "${args}" "${MAKE_FLAGS}" > ${LOGFILE}
}
function _compile {
  # parameters
  BUILD_DIR=${1}
  args=${2}  # QMake flags
  MAKE_FLAGS=${3}

  # generate translation files
  find . -name 'translation.ts' -exec lrelease -nounfinished -compress -removeidentical {} \;

  # set meta info
  find . -name "informations.xml" -exec sed -i -r "s;<architecture>.*</architecture>;<architecture>linux-x86_64-pc</architecture>;g" {} \;

  # version
  export ULTRACOPIER_VERSION=$(grep "ULTRACOPIER_VERSION" Version.h | sed 's/^#define\s\{1,\}ULTRACOPIER_VERSION\s\{1,\}"\([0-9]\{1,3\}\)\.\([0-9]\{1,3\}\)\.\([0-9]\{1,3\}\)\.\([0-9]\{1,3\}\)".*$/\1.\2.\3.\4/')
  # release
  if [ $DEBUG -ne 1 ]; then
    # TODO allow plugin verions different from core version
    find . -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \;
    find . -name "informations.xml" -exec sed -i -r "s/<pubDate>.*<\pubDate>/<pubDate>`date +%s`<\pubDate>/g" {} \;
  fi;

  # build
  if [ $DEBUG -eq 1 ]; then
    # all in one
    qmake ${args} "DESTDIR=\"$BUILD_DIR\"" ./ultracopier.pro
    make $MAKE_FLAGS
  else
    # full build
    function build_project {
      # parameters
      dst="${1}"
      pro="${2}"
      args=${3}
      flags=${4}
      # prepare destination
      old_pwd=$(pwd)
      mkdir -p "${dst}"
      cd "${dst}"
      # build
      eval qmake "${args}" "DESTDIR=\".\"" ${old_pwd}/${pro}  # NOTE eval needed to unpack $args
      res=$?  # save return code
      if [ $res != 0 ]; then
        cd "${old_pwd}"
        return $res
      fi;
      make ${flags}
      res=$?  # save return code
      cd "${old_pwd}"
      return $res
    }

    # TODO fix error:
    # mv -f libinterface.so ./
    # mv: 'libinterface.so' and './libinterface.so' are the same file
    # Makefile:169: recipe for target 'libinterface.so' failed
    # make: [libinterface.so] Error 1 (ignored)

    ## core
    echo "Building core..."
    build_project "${BUILD_DIR}/core" ./other-pro/ultracopier-core.pro "${args}" "${MAKE_FLAGS}"

    ## plugins
    function build_plugin {
      # variables
      args="${1}"
      target="${2}"
      plugin="${3}"
      plugin_path=$(dirname $plugin)
      plugin_file=$(basename $plugin)
      # compatibility
      if [[ $plugin_path == *'Windows'* ]]; then # TODO platform specific plugin list
        echo "Ignoring Windows plugin..."
        return
      fi
      # build
      echo "Building plugin $plugin..."
      build_project "${BUILD_DIR}/${plugin_path}" "${plugin}" "${args}" "${MAKE_FLAGS}"
      return $?
    }
    # add all plugin projects
    # NOTE ERROR "mv: 'libinterface.so' and './libinterface.so' are the same file" is because build_project() runs qmake DESTDIR="."
    # TODO parallel: subshell ()& + wait
    #SHOPT_OLD=$(shopt -p globstar)  # NOTE alternatively save and restore $SHELLOPTS $BASHOPTS
    #shopt -s globstar
    #for plugin in ./plugins/{,**/}*.pro; do
    #for plugin in ./plugins{,-unmaintained}/{,**/}*.pro; do  # include non-maintained plugins
    #  if [ "$(basename $plugin)" == '*.pro' ]; then
    #    continue
    #  fi
    #  build_plugin "${args}" "${BUILD_DIR}" ${plugin}
    #done
    #eval $SHOPT_OLD
    # selectively add projects
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/CopyEngine/Random/CopyEngine.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/CopyEngine/Rsync/CopyEngine.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/CopyEngine/Ultracopier/CopyEngine.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/CopyEngine/Ultracopier-Spec/CopyEngine.pro" #ALL-IN-ONE
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/Listener/catchcopy-v0002/listener.pro" #ALL-IN-ONE
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/PluginLoader/catchcopy-v0002/pluginLoader.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/SessionLoader/Windows/sessionLoader.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/Themes/Oxygen/interface.pro"
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/Themes/Oxygen2/interface.pro" #ALL-IN-ONE
    build_plugin "${args}" "${BUILD_DIR}" "./plugins/Themes/Supercopier/interface.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/Listener/dbus/listener.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/PluginLoader/keybinding/pluginLoader.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/SessionLoader/KDE4/sessionLoader.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/Themes/Clean/interface.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/Themes/Teracopy/interface.pro"
    #build_plugin "${args}" "${BUILD_DIR}" "./plugins-unmaintained/Themes/Windows/interface.pro"

    ## package (to build)
    # TODO only when everything before successful!
    echo "Packaging..."
    mkdir -p "${PACKAGE_DIR}"
    # TODO naming conventions for library file (*.so)
    # TODO automatic language detection
    ### executable
    echo "... executable ..."
    cp "${BUILD_DIR}/core/ultracopier" "${PACKAGE_DIR}/ultracopier" ## executable already there because of DESTDIR=
    ### resources
    echo "... resources ..."
    # NOTE files mentioned in the .qrc files are automatically embedded in the binary output files
    cp "./resources/ultracopier.desktop" "${PACKAGE_DIR}/"
    ### languages
    echo "... languages ..."
    # TODO auto detect languages
    LANGUAGES="ar de el es fr hi hu id it ja ko nl no pl pt ru th tr zh" # default: en
    function package_languages {
      langs_src="${1}/Languages"
      langs_dst="${2}/Languages"
      langs="${3}" # TODO language auto detection
      for lang in ${langs}; do
        mkdir -p "${langs_dst}/${lang}"
        cp "${langs_src}/${lang}/flag.png"         "${langs_dst}/${lang}"
        cp "${langs_src}/${lang}/informations.xml" "${langs_dst}/${lang}"
        cp "${langs_src}/${lang}/translation.qm"   "${langs_dst}/${lang}"
      done
    }
    #package_languages "./resources" "${PACKAGE_DIR}" "en" # NOTE packaged via resource file
    package_languages "./plugins"   "${PACKAGE_DIR}" "${LANGUAGES}"

    ### Plugins
    echo "... plugins ..."
    function package_plugin_languages {
      langs_src="${1}/Languages"
      langs_dst="${2}/Languages"
      langs="${3}" # TODO language auto detection
      for lang in ${langs}; do
        mkdir -p "${langs_dst}/${lang}"
        cp "${langs_src}/${lang}/translation.qm" "${langs_dst}/${lang}"
      done
    }
    function package_plugin {
      PLUGIN_TYPE="${1}"
      PLUGIN_NAME="${2}"
      PLUGIN_PATH="${PLUGIN_TYPE}/${PLUGIN_NAME}"
      PLUGIN_SRC="./plugins/${PLUGIN_PATH}" # TODO allow plugins_alternative
      PLUGIN_BUILD_SRC="${BUILD_DIR}/plugins/${PLUGIN_PATH}"
      PLUGIN_RELEASE_DST="${PACKAGE_DIR}/${PLUGIN_PATH}"
      PLUGIN_LIB_NAME="${3}" # TODO naming convention --> based on PLUGIN_TYPE
      LANGUAGES="${4}" # TODO language auto detection
      echo "  - $PLUGIN_NAME"
      mkdir -p "${PLUGIN_RELEASE_DST}"
      cp "${PLUGIN_SRC}/informations.xml"            "${PLUGIN_RELEASE_DST}"
      cp "${PLUGIN_BUILD_SRC}/${PLUGIN_LIB_NAME}.so" "${PLUGIN_RELEASE_DST}"
      package_plugin_languages "${PLUGIN_SRC}" "${PLUGIN_RELEASE_DST}" "${LANGUAGES}" "${LANGUAGES}"
    }
    package_plugin "CopyEngine"    "Ultracopier-Spec" "libcopyEngine"    "${LANGUAGES}"
    package_plugin "CopyEngine"    "Random"           "libcopyEngine"    ""
    package_plugin "CopyEngine"    "Ultracopier"      "libcopyEngine"    "${LANGUAGES}"
    package_plugin "CopyEngine"    "Rsync"            "libcopyEngine"    ""
    cp -r "${PACKAGE_DIR}/CopyEngine/Ultracopier/Languages" "${PACKAGE_DIR}/CopyEngine/Rsync/Languages" # NOTE inline code switches are used # TODO separate them completly!
    package_plugin "Listener"      "catchcopy-v0002"  "liblistener"      ""
    #package_plugin "PluginLoader"  "catchcopy-v0002"  "libpluginLoader"  "${LANGUAGES}" # NOTE Windows only
    #package_plugin "SessionLoader" "KDE4"             "libsessionLoader" ""
    #package_plugin "SessionLoader" "Windows"          "libsessionLoader" "" # NOTE Windows only
    package_plugin "Themes"        "Oxygen"           "libinterface"     "${LANGUAGES}"
    package_plugin "Themes"        "Oxygen2"          "libinterface"     "${LANGUAGES}"
    package_plugin "Themes"        "Supercopier"      "libinterface"     ""

    echo "... done!"
  fi
}
compile "$@"