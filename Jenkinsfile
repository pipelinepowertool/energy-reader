pipeline {

  agent any
  stages {
    stage('Build against Ubuntu') {
      agent {
        docker {
            image 'sdenboer/ubuntu-make:latest'
        }
      }
      steps {
        sh 'make'
        sh 'mv build/energy_reader build/energy_reader-default'
        withAWS(region:'eu-north-1',credentials:'jenkins-s3') {
          sh 'echo "Uploading content with AWS creds"'
          s3Upload(file:"$./build/energy_reader-default", bucket:'energy-reader')
        }
      }
    }
    stage('Build against Alpine Jenkins SSH Agent') {
      agent {
        docker {
            image 'sdenboer/jenkins-alpine-make:latest'
            args '-u root:root'
        }
      }
      steps {
        sh 'make'
        sh 'mv build/energy_reader build/energy_reader-jenkins-alpine'
        withAWS(region:'eu-north-1',credentials:'jenkins-s3') {
          sh 'echo "Uploading content with AWS creds"'
          s3Upload(file:"./build/energy_reader-jenkins-alpine", bucket:'energy-reader')
        }
      }
    }
  }
}