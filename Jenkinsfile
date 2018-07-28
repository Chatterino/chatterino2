pipeline {
    agent any

    stages {
        stage('Build') {
            parallel {
                stage('GCC') {
                    sh 'mkdir -p build && cd build && qmake .. && make'
                }
                stage('Clang') {
                    sh 'mkdir -p build && cd build && qmake -spec linux-clang .. && make'
                }
            }
        }
    }
}
