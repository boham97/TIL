```yml
name: -Service Deploy

on:
  push:
    branches:
      - main
    paths:
      - '-service/**'    #해당 서비스 아래 파일과 디렉토리 변화가 있다면

jobs:
  deploy-service-1:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout repository
        uses: actions/checkout@v2

      - name: Copy Jar
        run: cp ./build/libs/*.jar ./deploy

      - name: Make zip file
        run: zip -r ./springboor-with-githubaction.zip ./deploy

      - name: Awsaccess
        uses: aws-actions/configure-aws-credentials@v1
        with:
          aws-access-key-id: ${{ secrets.AWS_ACCESS_KEY }}
          aws-secret-access-key: ${{ secrets.AWS_SECRET_KEY }}
          aws-region: ${{ secrets.AWS_REGION }}
      - name: Upload to S3
        run: aws s3 cp --region ${{ secrets.AWS_REGION }} ./$GITHUB_SHA.zip s3://${{ secrets.S3_BUCKET_NAME }}/-service.zip

      - name: Code Deploy
        run: aws deploy create-deployment \
            --application-name ${{ secrets.CODEDEPLOY_NAME }} \
            --deployment-config-name CodeDeployDefault.AllAtOnce \
            --deployment-group-name ${{ secrets.CODEDEPLOY_GROUP }} \
            --s3-location bucket=${{ secrets.S3_BUCKET_NAME }},bundleType=zip,key=-service.zip.zip
```

# 순서

1. aws 관련 정보 github setting - secrets - actions에 저장

2. `jar { enabled = false}`

3. deploy 폴더생성 후  script파일 저장(codedeploy lifecycle 확인)

4. jar파일 복사후 deploy 폴더에 저장

5. 파일 압축

6. aws 접근

7. s3에 zip파일 업로드 (jar파일 인식 못해서)

8. codedeploy 배포 생성

script 파일엔 도커 이미지 빌드 및 실행 명령어

무중단 스프링 클라우드

[[Linux] Graceful Shutdown이란? (feat. SIGINT/SIGTERM/SIGKILL + signal handler) — 드프 DrawingProcess](https://csj000714.tistory.com/518)

**# Spring Cloud, Ansible, Docker를 사용해 서버 무중단 배포 Trouble Shooting**

정지후 종료사이에 잠시 기다리자

```bash

```



리트라이 필터를 넣자!

https://velog.io/@salgu1998/Spring-Cloud-Ansible-Docker%EB%A5%BC-%EC%82%AC%EC%9A%A9%ED%95%B4-%EC%84%9C%EB%B2%84-Rolling-update%EB%AC%B4%EC%A4%91%EB%8B%A8-%EB%B0%B0%ED%8F%AC-Trouble-Shooting

https://hudi.blog/zero-downtime-deployment/

롤링

컨테이너 하나더 생성후 다른 컨테이너 끄고 키고 반복후 새로 생성한 컨테이너 다운

![](C:\Users\bosung\Desktop\rolling-deployment-2-f9d7e1eae7c7a5be1629fd5dda3c011b.gif)
