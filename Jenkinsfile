pipeline {
    agent none

    environment {
        FW_ARTIFACT = "firmware.bin"
    }

    stages {

        stage('Checkout Source') {
            agent { label 'fw-build' }   // 🚨 Xeon build agent
            steps {
                checkout scm
            }
        }

        stage('Build ESP32 Firmware (Xeon)') {
            agent { label 'fw-build' }   // 🚨 build ONLY here
            steps {
                sh '''
                    idf.py set-target esp32
                    idf.py build
                    cp build/*.bin ${FW_ARTIFACT}
                '''
            }
        }

        stage('Archive Firmware Artifact (Xeon)') {
            agent { label 'fw-build' }
            steps {
                archiveArtifacts artifacts: "${FW_ARTIFACT}", fingerprint: true
            }
        }

        stage('Generate ATS Manifest (Xeon)') {
            agent { label 'fw-build' }
            steps {
                sh '''
                    SHA256=$(sha256sum ${FW_ARTIFACT} | awk '{print $1}')

                    cat <<EOF > ats-manifest.yaml
manifest_version: 1

build:
  ci_system: jenkins
  job_name: ${JOB_NAME}
  build_number: ${BUILD_NUMBER}
  git:
    repo: ${GIT_URL}
    commit: ${GIT_COMMIT}
    branch: ${GIT_BRANCH}
  artifact:
    name: ${FW_ARTIFACT}
    checksum: sha256:${SHA256}
    build_node: xeon-fw-build

device:
  target: esp32
  board: esp32-devkit

test_plan:
  - gpio_toggle_test
  - oled_display_test

timestamps:
  build_time: "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
EOF
                '''
                archiveArtifacts artifacts: 'ats-manifest.yaml'
            }
        }

        stage('Flash & Test on ATS Node (Pi)') {
            agent { label 'ats-node' }   // 🚨 Pi node
            steps {
                copyArtifacts(
                    projectName: env.JOB_NAME,
                    selector: specific("${env.BUILD_NUMBER}"),
                    filter: "${FW_ARTIFACT}"
                )

                sh '''
                    echo "[ATS] Flashing firmware from Xeon artifact"
                    ./agent/flash_fw.sh ${FW_ARTIFACT}

                    echo "[ATS] Running hardware tests"
                    ./agent/run_tests.sh
                '''
            }
        }
    }

    post {
        success {
            echo "✅ Firmware built on Xeon and validated on ATS node"
        }
        failure {
            echo "❌ Firmware validation failed"
        }
        always {
            archiveArtifacts artifacts: 'reports/**', allowEmptyArchive: true
        }
    }
}
