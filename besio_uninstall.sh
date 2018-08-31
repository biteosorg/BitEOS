#! /bin/bash

binaries=(clbes
          besio-abigen
          besio-launcher
          besio-s2wasm
          besio-wast2wasm
          besiocpp
          kbesd
          nodbes
          besio-applesdemo)

if [ -d "/usr/local/besio" ]; then
   printf "\tDo you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\n\tThis requires sudo, please run ./besio_uninstall.sh with sudo\n\n"
               exit -1
            fi

            pushd /usr/local &> /dev/null
            rm -rf besio
            pushd bin &> /dev/null
            for binary in ${binaries[@]}; do
               rm ${binary}
            done
            popd &> /dev/null
            break;;
         [Nn]* ) 
            printf "\tAborting uninstall\n\n"
            exit -1;;
      esac
   done
fi
