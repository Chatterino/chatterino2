pipeline {
    agent any

    stages {
        stage('Build') {
            parallel {
                stage('GCC') {
                    steps {
                        sh 'mkdir -p build-linux-gcc && cd build-linux-gcc && qmake .. && make'
                    }
                }
                stage('Clang') {
                    steps {
                        sh 'mkdir -p build-linux-clang && cd build-linux-clang && qmake -spec linux-clang .. && make'
                    }
                }
            }
        }
    }
}
