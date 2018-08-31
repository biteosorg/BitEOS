#!/bin/bash
##########################################################################
# This is the BESIO automated install script for Linux and Mac OS.
# This file was downloaded from https://github.com/biteosorg/BitEOS
#
# Copyright (c) 2017, Respective Authors all rights reserved.
#
# After June 1, 2018 this software is available under the following terms:
# 
# The MIT License
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# https://github.com/biteosorg/BitEOS/LICENSE.txt
##########################################################################
   

   CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
   if [ "${CWD}" != "${PWD}" ]; then
      printf "\\n\\tPlease cd into directory %s to run this script.\\n \\tExiting now.\\n\\n" "${CWD}"
      exit 1
   fi

   BUILD_DIR="${PWD}/build"
   CMAKE_BUILD_TYPE=Release
   TIME_BEGIN=$( date -u +%s )
   INSTALL_PREFIX="/usr/local/besio"
   VERSION=1.2

   txtbld=$(tput bold)
   bldred=${txtbld}$(tput setaf 1)
   txtrst=$(tput sgr0)

   create_symlink() {
      pushd /usr/local/bin &> /dev/null
      ln -sf ../besio/bin/$1 $1
      popd &> /dev/null
   }

   install_symlinks() {
      printf "\\n\\tInstalling BESIO Binary Symlinks\\n\\n"
      create_symlink "clbes"
      create_symlink "besio-abigen"
      create_symlink "besio-launcher"
      create_symlink "besio-s2wasm"
      create_symlink "besio-wast2wasm"
      create_symlink "besiocpp"
      create_symlink "kbesd"
      create_symlink "nodbes"
   }

   if [ ! -d "${BUILD_DIR}" ]; then
      printf "\\n\\tError, besio_build.sh has not ran.  Please run ./besio_build.sh first!\\n\\n"
      exit -1
   fi

   ${PWD}/scripts/clean_old_install.sh
   if [ $? -ne 0 ]; then
      printf "\\n\\tError occurred while trying to remove old installation!\\n\\n"
      exit -1
   fi

   if ! pushd "${BUILD_DIR}"
   then
      printf "Unable to enter build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
      exit 1;
   fi
   
   if ! make install
   then
      printf "\\n\\t>>>>>>>>>>>>>>>>>>>> MAKE installing BESIO has exited with the above error.\\n\\n"
      exit -1
   fi
   popd &> /dev/null 

   install_symlinks   

   printf "\n\n${bldred}\t _______   _________ _________  _______   _______   _______ \n"         
   printf "\t(  ____ \  \__   __/ \__   __/ (  ____ \ (  ___  ) (  ____ \\\\\n"         
   printf "\t| (    ) |    ) (       ) (    | (    \/ | (   ) | | (    \/\n"         
   printf "\t| (____)/     | |       | |    | (__     | |   | | | (_____ \n"         
   printf "\t|  ___(       | |       | |    |  __)    | |   | | (_____  )\n"         
   printf "\t| (    )\     | |       | |    | (       | |   | |       ) |\n"         
   printf "\t| (____) | ___) (___    | |    | (____/\ | (___) | /\____) |\n"         
   printf "\t(_______/  \_______/    |_|    (_______/ (_______) \_______)\n${txtrst}"


   printf "\\tFor more information:\\n"
   printf "\\tBESIO website: http://biteos.org\\n"
   printf "\\tBESIO Telegram channel @ https://t.me/BESProject\\n"
   printf "\\tBESIO resources: http://biteos.org/resources/\\n"
   printf "\\tBESIO Stack Exchange: https://besio.stackexchange.com\\n"
   printf "\\tBESIO wiki: https://github.com/biteosorg/BitEOS/wiki\\n\\n\\n"
