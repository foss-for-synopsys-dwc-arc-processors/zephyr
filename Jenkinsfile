pipeline {
  agent any
  options {
      timeout(time: 4, unit: 'HOURS') 
      buildDiscarder(
          logRotator(daysToKeepStr: '3', numToKeepStr:'5'))
  }
  triggers {
      // cron('0 1 * * *')
      gitlab(triggerOnPush: true, triggerOnMergeRequest: true, branchFilterType: 'All')
  }
  environment {
    ZEPHYR_TOOLCHAIN_VARIANT="zephyr"
    SDK="0.10.3"
    ZEPHYR_SDK_INSTALL_ROOT="${env.WORKSPACE}/zephyr-sdk-files"
    ZEPHYR_SDK_INSTALL_DIR="${env.WORKSPACE}/zephyr-sdk"
  }

  stages {

    stage('Run on nism') {
      parallel {
        stage('1') {
          agent { 
              node { 
                  label 'gp&&linux&&us01' 
              } 
          }
          steps {
            before_install()
            build_script()
            script{
              archiveArtifacts artifacts: '**/*.csv', fingerprint: true
              archiveArtifacts artifacts: '**/*.log', fingerprint: true
            }
          }

        }
        stage('2') {
          agent { 
              node { 
                  label 'gp&&linux&&us01' 
              } 
          }
          steps {
            before_install()
            build_script()
            script{
              archiveArtifacts artifacts: '**/*.csv', fingerprint: true
              archiveArtifacts artifacts: '**/*.log', fingerprint: true
            }
          }

        }
        stage('3') {
          agent { 
              node { 
                  label 'gp&&linux&&us01' 
              } 
          }
          steps {
            before_install()
            build_script()
            script{
              archiveArtifacts artifacts: '**/*.csv', fingerprint: true
              archiveArtifacts artifacts: '**/*.log', fingerprint: true
            }
          }

        }
        stage('4') {
          agent { 
              node { 
                  label 'gp&&linux&&us01' 
              } 
          }
          steps {
            before_install()
            build_script()
            script{
              archiveArtifacts artifacts: '**/*.csv', fingerprint: true
              archiveArtifacts artifacts: '**/*.log', fingerprint: true
            }
          }

        }
      }
    }
    stage('Deploy') {
        steps {
          build job: 'zephyrproject-rtos/zephyr_deploy', parameters: [string(name: 'U_JOB_NAME', value: env.JOB_NAME),string(name: 'GIT_COMMIT', value: env.BUILD_NUMBER),string(name: 'GIT_URL', value: env.GIT_URL),string(name: 'GIT_COMMIT', value: env.GIT_COMMIT)]  //this is where we specify which job to invoke.

        }
    }
  }
  // post { 
  //     always { 
  //         cleanWs()
  //     }
  // } 

}

void before_install() {
    sh '''
      source /global/etc/modules.sh
      module load arcnsim
      module load gmake/3.82
      module load python/3.7.0
      python -m pip install --upgrade pip --user 
      python -m pip install west --user 
      python -m pip install serial --user 
      python -m pip install ply --user 
      python -m pip install cmake --user 
      python -m pip install wheel --user 
      python -m pip install PyYAML --user 
      python -m pip install gitlint --user 
      python -m pip install pyelftools --user 
      python -m pip install pyserial --user
      python -m pip install XlsxWriter --user
      python -m pip install chardet --user

      if [ "$STAGE_NAME" != "Deploy" ]; then
        
        if [ ! -d "cur_dtc" ]; then
          echo "install dtc and gpref"
          mkdir -p cur_dtc
          cd cur_dtc
          if [ ! -f "dtc-1.4.6-1.el7.x86_64.rpm" ]; then
            wget http://mirror.centos.org/centos/7/extras/x86_64/Packages/dtc-1.4.6-1.el7.x86_64.rpm -q
          fi
          rpm2cpio dtc-1.4.6-1.el7.x86_64.rpm | cpio -di

          if [ ! -f "gperf-3.0.4-8.el7.x86_64.rpm" ]; then
            wget http://mirror.centos.org/centos/7/os/x86_64/Packages/gperf-3.0.4-8.el7.x86_64.rpm -q
          fi
          rpm2cpio gperf-3.0.4-8.el7.x86_64.rpm | cpio -di
          cd ..
        fi

        if [ ! -d "$WORKSPACE/zephyr-sdk" ]; then
          # install zephyr sdk
          if [ ! -f "zephyr-sdk-${SDK}-setup.run" ]; then

            wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK}/zephyr-sdk-${SDK}-setup.run # -q
          fi
          sh zephyr-sdk-${SDK}-setup.run --target zephyr-sdk-files --noexec
          sed -in-place -e 's/-xf/-jxvf/g' zephyr-sdk-files/setup.sh

          cd zephyr-sdk-files
          sh setup.sh -d $WORKSPACE/zephyr-sdk
          cd ..
        fi

        # To save disk
        find ${WORKSPACE} -iname '*.rst' -delete
        find ${WORKSPACE} -iname '*.md' -delete
        rm -rf ${WORKSPACE}/doc
        rm -rf zephyr-sdk-${SDK}-setup.run
        rm -rf zephyr-sdk-files
        rm -rf ${WORKSPAC}/cur_dtc/*.rpm
        rm -rf ${WORKSPAC}/tests/kernel
      fi
      echo "$WORKSPACE/cur_dtc/usr/bin:$HOME/.local/bin:$PATH" > env.prop
    '''
}
void build_script() {
  sh '''
    MATRIX=${STAGE_NAME}

    INJECT_PATH=$(cat $WORKSPACE/env.prop)
    export ZEPHYR_SDK_INSTALL_DIR=${WORKSPACE}/zephyr-sdk
    export PATH=$INJECT_PATH
    export LD_LIBRARY_PATH="/global/freeware/Linux/RHEL6/python-3.7.0/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/tcl-8.6.8/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/tk-8.6.8/lib:/global/freeware/Linux/RHEL6/python-3.7.0/libs:/global/freeware/Linux/RHEL6/glibc-2.14/lib:$LD_LIBRARY_PATH"
    cd ..
    if [ -d ".west" ]; then
      rm -rf .west
    fi
    NAME=${WORKSPACE##*/}
    west init -l $NAME
    west update || true
    cd $NAME
    source zephyr-env.sh

    nsim_platform=$(find "${ZEPHYR_BASE}/boards/arc/nsim" -name "*_defconfig" \
    | while read file; do
    echo "$(basename ${file%_defconfig})"
    done \
    | uniq )

    SANITYCHECK="${ZEPHYR_BASE}/scripts/sanitycheck"
    if [ ! -d "archive" ]; then
      mkdir archive
    fi

    for i in $nsim_platform
    do
        ${SANITYCHECK} -p ${i} -T tests --subset ${MATRIX}/4 -O nsim -o ${i}_result.csv || true
        while IFS= read -r line; do
          IFS=', ' read -r -a array <<< "$line"
          if [ "${array[3]}" == "False" ]; then
            find nsim/${i}/${array[0]} -iname handler.log | while read file; do mv "${file}" archive/"${file//[\\/]/_}"; done
          fi
        done < "${i}_result.csv"
        mv ${i}_result.csv archive/${i}_result.csv
    done 
    echo PATH="$WORKSPACE/cur_dtc/usr/bin:$HOME/.local/bin:$PATH" >> env.prop
    echo LD_LIBRARY_PATH="/global/freeware/Linux/RHEL6/python-3.7.0/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/tcl-8.6.8/lib:/global/freeware/Linux/RHEL6/python-3.7.0/deps/tk-8.6.8/lib:/global/freeware/Linux/RHEL6/python-3.7.0/libs:/global/freeware/Linux/RHEL6/glibc-2.14/lib:$LD_LIBRARY_PATH" >> env.prop

  '''
}
