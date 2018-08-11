pipeline {
    agent any

    stages {
        stage('Build') {
            parallel {
                stage('GCC') {
                    steps {
                        sh 'mkdir -p build-linux-gcc && cd build-linux-gcc && make distclean; qmake .. && make'
                    }
                }
                stage('Clang') {
                    steps {
                        sh 'mkdir -p build-linux-clang && cd build-linux-clang && make distclean; qmake -spec linux-clang .. && make'
                    }
                }
            }
        }
    }
}
